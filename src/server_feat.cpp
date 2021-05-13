#include "shash_class.h"
#include "fir_api.h"
#include "typedef.h"

#include "audio_db.h"
// #include "proxy.pb.h"
#include "audio_feature.pb.h"

using namespace bigoai;

/* -- Deployment tools -- */
DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, 300, "Connection will be closed if there is no read/write operations during the last `idle_timeout_s'");
DEFINE_bool(use_auto_limiter, false, "");
DEFINE_string(service_name, "hello-shash-feat", "Original's was charlie's likee-audio-audit");
DEFINE_int32(num_decoder, 32, "");
DEFINE_int32(num_hasher, 32, "");
DEFINE_int32(sample_rate, 8000, "");
DEFINE_bool(log_request, false, "whether dump request attachment.");
/* -- shash parameter -- */
DEFINE_double(threshold_db,  1, "thabs_db = 1");
DEFINE_double(threshold_qry, 8192, "\
    0.25 if float, \
    (0.25 * 2^15 = 8192) if short, \
    0.00002864154 if use qrywavpath2audiovector()");
BRPC_VALIDATE_GFLAG(log_request, brpc::PassValidate);
moodycamel::BlockingConcurrentQueue<std::unique_ptr<FFMpegVideoDecoder>> g_decoders;
moodycamel::BlockingConcurrentQueue<std::unique_ptr<AudioSHasher>> g_shashers; // g_yhashers g_yhashers g_yhashers g_yhashers


/* -- functions -- */
template <typename T>
class QueueBorrower {
    moodycamel::BlockingConcurrentQueue<T>& que_;
    T obj_;
public:
    QueueBorrower(const QueueBorrower<T>&) = delete;

    QueueBorrower(moodycamel::BlockingConcurrentQueue<T>& que) : que_(que) {
        // LOG(INFO) << __func__ << "(): borrow a " << typeid(T).name();
        que.wait_dequeue(obj_);
    }

    ~QueueBorrower() {
        que_.enqueue(std::move(obj_));
        // LOG(INFO) << __func__ << "(): return a " << typeid(T).name();
    }

    T& get() {
        return obj_;
    }
};

class LogRAII {
public:
    LogRAII(const brpc::Controller* cntl, std::string msg) : cntl_(cntl), msg_(msg) {
        LOG(INFO) << __func__ << "(): ++ request[log_id=" << cntl_->log_id() << "] from " << cntl_->remote_side() << ", msg[" << msg_
                            << "]";
    }

    ~LogRAII() {
        LOG(INFO) << __func__ << "(): -- request[log_id=" << cntl_->log_id() << "] from " << cntl_->remote_side() << ", msg[" << msg_
                            << "]";
    }
private:
    const brpc::Controller* cntl_ = nullptr; // weak ptr
    std::string msg_;
};

int decode_audio(
    std::unique_ptr<FFMpegVideoDecoder>& d,
    brpc::Controller* cntl,
    const Request* req,
    std::vector<float>* audio_data) {

    /* Convert Video/Audio in binary/bytes to vector form

        Inputs
        ------
        d : std::unique_ptr<FFMpegVideoDecoder>&

            One of the ffmpeg decoder.
            This is only used when input mp4 from likee. This decodes the video into audio.
            Will not be used for hello purposes.

        cntl : brpc::Controller*

            controller that stores output information

            e.g:
            ----
            cntl->http_response().set_status_code(503); # Decoding error from video 
            cntl->http_response().set_status_code(400); # Decoding error from audio or Unknown error

        req : const Request*

            Input details to be processed.

            e.g:
            ----
            req->has_video() : True if we input mp4/likee videos

            req->video() : Binary video data

            req->data().data() : Audio data bytes
                ```python
                def load_pcm_s16_sr16000(path):
                    y, sr = librosa.load(path, mono=True, sr=16000, res_type="kaiser_fast")
                    y = y * 32768
                    y = y.astype(np.int16)
                    return y.tobytes()
                data = load_pcm_s16_sr16000(path)
                request['data'] = base64.b64encode(data).decode("ascii")
                ```

            req->data().size() : Number of audio samples


        Returns
        -------
        audio_data : std::vector<float>*

            Downsampled audio samples from 16k to 8k
    */

    try {

        /* Video decoding from Likee */
        if (req->has_video()) {
            // LOG(INFO) << __func__ << "(): decoding video...";
            const auto& v = req->video();
            if (0 != d->DecodeAudio(reinterpret_cast<const uint8_t*>(v.data()), v.size(), FLAGS_sample_rate, 1, audio_data)) {
                cntl->http_response().set_status_code(503);
                cntl->request_attachment().append("decoding video failure");
                LOG(ERROR) << __func__ << "(): decoding video failure";
                return -1;
            }
            cntl->http_response().set_status_code(200);
            cntl->request_attachment().append("decoding video success");
        }

        /* Audio decoding from Hello */
        else {
            // LOG(INFO) << __func__ << "(): decoding audio...";

            /* Get the audio duration */
            size_t nbytes = req->data().size();
            if ((nbytes % sizeof(int16_t)) != 0) {
                auto err_msg = "Bad Request: invalid data length, len(data) % sizeof(int16_t) != 0";
                LOG(ERROR) << __func__ << "(): " << err_msg;
                cntl->http_response().set_status_code(503);
                cntl->request_attachment().append(err_msg);
                return 400;
            }
            const int numel = nbytes / sizeof(int16_t);
            LOG(INFO) << __func__ << "(): Duration = " << (float) numel/FLAGS_sample_rate << ", numel = " << numel;
            /* Insufficient duration */
            if (numel <= 3 * FLAGS_sample_rate) {
                auto err_msg = "WARNING : len(audio) <= 3 seconds !!!";
                LOG(INFO) << __func__ << "(): " << err_msg;
            }
            /* Exceed duration */
            if (numel >= 60 * FLAGS_sample_rate) {
                auto err_msg = "WARNING : len(audio) >= 60 seconds !!!";
                LOG(INFO) << __func__ << "(): " << err_msg;
            }

            /* Get the audio bytes to pcm */
            const char* bytes = req->data().data();
            auto pcm = reinterpret_cast<const int16_t*>(bytes);

            /* save to audio_data vector */
            for (int i = 0; i < numel; i++) {
                audio_data->push_back( static_cast<float>(pcm[i]) );
            }

        }
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        LOG(INFO) << __func__ << "(): decoding audio failure : e.what() = " << e.what();
        cntl->request_attachment().append("decoding audio failure");
        cntl->http_response().set_status_code(400);
        return -1;
    }

    return 0;
}

int compute_shash(
    brpc::Controller* cntl,
    const Request* request,
    float threshold_abs,
    std::unordered_map <int, std::set<int>>* shash) {

    /* Compute a single shash from the controller request

        Inputs
        ------

        cntl : brpc::Controller*

            controller that stores output information

            e.g:
            ----
            cntl->http_response().set_status_code(500); # decode_audio() or get_shash() failed

        req : const Request*

            Input details to be processed.

            e.g:
            ----
            req->has_video() : True if we input mp4/likee videos

            req->video() : Binary video data

            req->data().data() : Audio data bytes
                ```python
                def load_pcm_s16_sr16000(path):
                    y, sr = librosa.load(path, mono=True, sr=16000, res_type="kaiser_fast")
                    y = y * 32768
                    y = y.astype(np.int16)
                    return y.tobytes()
                data = load_pcm_s16_sr16000(path)
                request['data'] = base64.b64encode(data).decode("ascii")
                ```

            req->data().size() : Number of audio samples

        threshold_abs : float
            threshold for peak selection


        Returns
        -------

        shash : std::unordered_map <int, std::set<int>>*
            pointer to an the shash.
    */

    /* Get audio data */
    LOG(INFO) << __func__ << "(): " << "Get audio data";
    std::vector<float> audio_data;
    std::string ErrorMessage;
    if (0 != 
            decode_audio(
                QueueBorrower<std::unique_ptr<FFMpegVideoDecoder>>(g_decoders).get(),
                cntl,
                request,
                &audio_data)) {
        ErrorMessage = "failed to run decode_audio() successfully.";
        cntl->http_response().set_status_code(500);
        LOG(ERROR) << __func__ << "(): " << ErrorMessage;
        cntl->request_attachment().append(ErrorMessage);
        return -1;
    }

    // LOG(INFO) << __func__ << "(): " << "audio_data.size() = " << audio_data.size();
    // LOG(INFO) << __func__ << "(): " << "Number of seconds = " << (float) audio_data.size()/FLAGS_sample_rate;
    // LOG(INFO) << __func__ << "(): " << "sample_rate = " << FLAGS_sample_rate;

    /* Get shash */
    LOG(INFO) << __func__ << "(): " << "Get shash";
    if (true != 
        QueueBorrower<std::unique_ptr<AudioSHasher>>(g_shashers).get()->get_shash(
            audio_data,
            FLAGS_sample_rate,
            threshold_abs,
            *shash) ) {
        ErrorMessage = "failed to compute shash !";
        cntl->http_response().set_status_code(500);
        LOG(ERROR) << __func__ << "(): " << ErrorMessage;
        cntl->request_attachment().append(ErrorMessage);
        return -1;
    }

    cntl->http_response().set_status_code(200);
    return 0;
}

int compute_shashes(
    brpc::Controller* cntl,
    const Request* request,
    float threshold_abs,
    std::vector< std::unordered_map <int, std::set<int>> > *shashes) {

    /* Same as compute_shash(), except that instead of computing the shash for just the full audio.
    Break up the audio data into chunks of 1 min each, and perform shash on each 1 min segment.

        Inputs
        ------

        Refer to compute_shash().


        Returns
        -------

        shashes : std::vector< std::unordered_map <int, std::set<int>> > *
            pointer to the shashes.
    */

    /* Get audio data */
    LOG(INFO) << __func__ << "(): " << "Get audio data";
    std::vector<float> audio_data;
    std::string ErrorMessage;
    if (0 != 
            decode_audio(QueueBorrower<std::unique_ptr<FFMpegVideoDecoder>>(g_decoders).get(),
                cntl,
                request,
                &audio_data)) {
        ErrorMessage = "failed to run decode_audio() successfully.";
        cntl->http_response().set_status_code(500);
        LOG(ERROR) << __func__ << "(): " << ErrorMessage;
        cntl->request_attachment().append(ErrorMessage);
        return -1;
    }

    // LOG(INFO) << __func__ << "(): " << "audio_data.size() = " << audio_data.size();
    // LOG(INFO) << __func__ << "(): " << "Number of seconds = " << (float) audio_data.size()/FLAGS_sample_rate;
    // LOG(INFO) << __func__ << "(): " << "sample_rate = " << FLAGS_sample_rate;

    /* Get shashes */
    LOG(INFO) << __func__ << "(): " << "Get shashes";
    if (true != 
        QueueBorrower<std::unique_ptr<AudioSHasher>>(g_shashers).get()->get_shashes(
            audio_data,
            FLAGS_sample_rate,
            threshold_abs,
            *shashes) ) {
        ErrorMessage = "failed to compute shash !";
        cntl->http_response().set_status_code(500);
        LOG(ERROR) << __func__ << "(): " << ErrorMessage;
        cntl->request_attachment().append(ErrorMessage);
        return -1;
    }

    cntl->http_response().set_status_code(200);
    return 0;
}


class AudioFeatureImpl : public AudioFeature {
public:
    AudioFeatureImpl() : db_client_() {
        LOG(INFO) << __func__ << "(): ================== ";
    };

    virtual ~AudioFeatureImpl(){
        LOG(INFO) << __func__ << "(): ================== ";
    };

    void write(brpc::Controller* cntl, const Request* request, HttpResponse* response) {
        LOG(INFO) << __func__ << "(): ================== ";
        LOG(INFO) << __func__ << "(): write op called. ";

        std::vector< std::unordered_map <int, std::set<int>> > shashes;
        if (0 != compute_shashes(cntl, request, FLAGS_threshold_db, &shashes)) {
            return;
        }

        LOG(INFO) << request;
        std::string label = "null";
        if (request->has_label()) {
            label = request->label();
        }
        db_client_.http_get_write(cntl, request->id(), shashes, label);

        LOG(INFO) << __func__ << "(): ================== ";
    }

    void query(brpc::Controller* cntl, const Request* request, HttpResponse* response) {
        LOG(INFO) << __func__ << "(): ================== ";
        LOG(INFO) << __func__ << "(): query op called. ";

        std::unordered_map <int, std::set<int>> shash;
        if (0 != compute_shash(cntl, request, FLAGS_threshold_qry, &shash)) {
            return;
        }
        db_client_.http_get_query(cntl, shash, request->id(), request->threshold_match());
    }

    void del(brpc::Controller* cntl, const Request* request, HttpResponse* response) {
        LOG(INFO) << __func__ << "(): ================== ";
        LOG(INFO) << __func__ << "(): del op called. ";

        db_client_.http_get_del(cntl, request->id());
    }

    void dump(brpc::Controller* cntl, HttpResponse* response) {
        LOG(INFO) << __func__ << "(): ================== ";
        LOG(INFO) << __func__ << "(): dump op called. ";

        db_client_.http_get_dump(cntl);
    }

    virtual void process(
        google::protobuf::RpcController* cntl_base,
        const Request* request,
        HttpResponse* response,
        google::protobuf::Closure* done) {

        /* Main process when taking request from kafka, and to transmit to serv

            Inputs:
            -------

            request : const Request*
                Inputs to this from kafka
                request->op()       : type of operation. if query, will run the self.query() function
                request->id()       : the vid of the input hello audio. MUST be a integer, if not there will be error.
                request->post_id()  : same as id, except Likee sends post_id instead of id.
                request->url()      : raw url of the hello audio.
                request->label()    : Details of the song.
                    If the operation is "add", this will be the song name.
                    If the operation is "query", this will be the order_id.
                request->threshold_match() : for database with number of match greater than this value, it will be considered as positive.

            Returns:
            --------

            cntl_base : google::protobuf::RpcController*
                Sends the information back to the controller or to whereever sends the requests.
                e.g:
                If you send any operation other than query/write/del/dump, we return 400 error.
                cntl->http_response().set_status_code(400);

            response : HttpResponse*
                Storage of the response of the respective functions.

            done : google::protobuf::Closure*
                ?
        */

        /* 1.0 Init */
        brpc::ClosureGuard done_guard(done);
        brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
        cntl->http_response().set_content_type("application/json");

        /* DEBUGGING : only run my inputs */
        if (request->uid().compare("3841557125") != 0) {
            std::string ErrMsg = "not my id, skipping...";
            LOG(ERROR) << __func__ << "(): " << ErrMsg;
            cntl->http_response().set_status_code(400);
            cntl->request_attachment().append(ErrMsg);
            return;
        }

        /* Verbose */
        try {
            LOG(INFO) << __func__ << "(): ========= Verbose ========= ";
            LOG(INFO) << __func__ << "(): request->op()         : " << request->op();
            LOG(INFO) << __func__ << "(): request->uid()        : " << request->uid();
            LOG(INFO) << __func__ << "(): request->id()         : " << request->id();
            LOG(INFO) << __func__ << "(): request->post_id()    : " << request->post_id();
            LOG(INFO) << __func__ << "(): request->url()        : " << request->url();
            LOG(INFO) << __func__ << "(): request->label()      : " << request->label();
            LOG(INFO) << __func__ << "(): request->threshold_match(): " << request->threshold_match();
        } catch (std::exception& e) {
            LOG(INFO) << __func__ << "(): e.what() = "<< e.what();
        }

        /* LOG : postid, url */
        std::string postid = request->post_id();
        if (request->has__vinfo()) {
            CHECK(request->_vinfo().shape_size());
            LOG(INFO) << __func__ << "(): request->postid: " << postid << ", request->_vinfo (frame num): " << request->_vinfo().shape(0);
        }

        std::string url = "null";
        if (request->has_url()) {
            url = request->url();
        }
        LogRAII log(cntl, "id:" + postid + ", " + url);
        LOG_IF(INFO, FLAGS_log_request) << "request attachment: " << cntl->request_attachment();

        /* request->op() = {write, query, del, dump} */
        if (request->op() == "write") {
            write(cntl, request, response);
            return;
        }
        else if (request->op() == "query") {
            query(cntl, request, response);
            return;
        }
        else if (request->op() == "del") {
            del(cntl, request, response);
            return;
        }
        else if (request->op() == "dump") {
            dump(cntl, response);
            return;
        }
        else {
            cntl->http_response().set_status_code(400);
            cntl->request_attachment().append("supported op: query/write/del/dump, but get " + request->op());
            LOG(ERROR) << "Unsupported op: " << request->op();
            return;
        }
    }

    virtual void ping(google::protobuf::RpcController* cntl_base,
        const HttpRequest* _,
        HttpResponse* response,
        google::protobuf::Closure* done) override {
        LOG(INFO) << __func__ << "(): ================== ";
        brpc::ClosureGuard done_guard(done);

        brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);

        cntl->response_attachment().append("OK\n");
    }
private:
    AudioDbClient db_client_;
};


/* -- main -- */
int main(int argc, char** argv){
    LOG(INFO) << "shash version 1.0";

    /* GFLAGS */
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    LOG(INFO) << "--";
    LOG(INFO) << "FLAGS_port              : " << FLAGS_port;
    LOG(INFO) << "FLAGS_idle_timeout_s    : " << FLAGS_idle_timeout_s;
    LOG(INFO) << "FLAGS_use_auto_limiter  : " << FLAGS_use_auto_limiter;
    LOG(INFO) << "FLAGS_service_name      : " << FLAGS_service_name;
    LOG(INFO) << "FLAGS_num_decoder       : " << FLAGS_num_decoder;
    LOG(INFO) << "FLAGS_num_hasher        : " << FLAGS_num_hasher;
    LOG(INFO) << "FLAGS_sample_rate       : " << FLAGS_sample_rate;
    LOG(INFO) << "FLAGS_log_request       : " << FLAGS_log_request;
    LOG(INFO) << "--";
    LOG(INFO) << "FLAGS_threshold_db      : " << FLAGS_threshold_db;
    LOG(INFO) << "FLAGS_threshold_qry     : " << FLAGS_threshold_qry;
    LOG(INFO) << "--";

    LOG(INFO) << __func__ << "(): ========= Initializing, add decoder ========= ";
    for(int i=0; i<FLAGS_num_decoder; ++i) {
        LOG(INFO) << "Initializing, add decoder" << i;
        auto d = std::make_unique<FFMpegVideoDecoder>();
        d->Init();
        g_decoders.enqueue(std::move(d));
    }

    LOG(INFO) << __func__ << "(): ========= Initializing, add shasher ========= ";
        for(int i=0; i<FLAGS_num_hasher; ++i) {
            LOG(INFO) << "Initializing, add shasher" << i;
            g_shashers.enqueue(std::make_unique<AudioSHasher>());
        }

    LOG(INFO) << __func__ << "(): ========= init server sim_audit ========= ";
    brpc::Server server;
    AudioFeatureImpl sim_audit;

    LOG(INFO) << __func__ << "(): ========= server.AddService() ========= ";
    if (server.AddService(  &sim_audit,
                            brpc::SERVER_DOESNT_OWN_SERVICE,
                            "/api/ping => ping,"
                            "/ping => ping,"
                            "/api/" + FLAGS_service_name + " => process") != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }
    LOG(INFO) << "start serv " << FLAGS_service_name;

    LOG(INFO) << __func__ << "(): ========= init srv_options ========= ";
    brpc::ServerOptions srv_options;
    srv_options.idle_timeout_sec = FLAGS_idle_timeout_s;
    LOG(INFO) << __func__ << "(): ========= FLAGS_use_auto_limiter ========= ";
    if (FLAGS_use_auto_limiter) {
        server.MaxConcurrencyOf("bigoai.process") = "auto";
    }
    LOG(INFO) << __func__ << "(): ========= server.Start() ========= ";
    if (server.Start(FLAGS_port, &srv_options) != 0) {
        LOG(ERROR) << "Fail to start server";
        return -1;
    }

    LOG(INFO) << __func__ << "(): ========= server.RunUntilAskedToQuit() ========= ";
    server.RunUntilAskedToQuit();
    return 0;


    printf("\n===============\n");
    printf(  "==== Done =====\n");
    printf(  "===============\n");
    return 0;
}
