#include "submodule/label_control_submodule.h"


using namespace bigoai;


LabelControlSubModule::LabelControlSubModule() {}

bool LabelControlSubModule::init(const SubModuleConfig &conf) {
    //init log_id rand generator 
    srand((unsigned)time(NULL));
    
    // 1.0 Init channel
    url_ = conf.rpc().url();

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    channel_manager_.set_retry_time(conf.rpc().max_retry());

    model_keys_ = conf.basic().model_keys();
    model_keys_list_ = split(model_keys_, ',');
    CHECK_GE(model_keys_list_.size(), 1u);
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }

    appid_ = conf.basic().appid();
    model_types_ = conf.basic().model_types();
    class_num_ = conf.basic().class_num();
    // use to skip the upload about normal branch
    normal_label_index_ = conf.basic().normal_label_index();

    // the above params are used to control label 
    for (auto mapping_indice : conf.label_control_conf().mapping_conf().mapping_indices()) {
        mapping_indices_list_.push_back(mapping_indice);
    }
    for (auto aggragation_type: conf.label_control_conf().mapping_conf().aggragation_types()){
        aggragation_types_list_.push_back(aggragation_type);
    }
    mapped_class_num_ = conf.label_control_conf().mapped_conf().mapped_class_num();
    mapped_model_keys_ = conf.label_control_conf().mapped_conf().mapped_model_keys();
    mapped_model_keys_list_ = split(mapped_model_keys_, ',');
    for (auto selected_indice : conf.label_control_conf().selected_indices()) {
        selected_indices_list_.push_back(selected_indice);
    }

    LOG(INFO) << "Init " << this->name();
    LOG(INFO) << "Append model keys " << model_keys_;
    LOG(INFO) << "Append model keys after label mapping " << mapped_model_keys_;
    return true;
}

bool LabelControlSubModule::call_service(ContextPtr &ctx)
{
    std::vector<ContextPtr> batch{ctx};
    return call_service_batch(batch);
}

bool LabelControlSubModule::call_service_batch(std::vector<ContextPtr> &ctxs)
{   
    resp_.Clear();  //must do this step
    fail_.resize(ctxs.size(), 0);

    std::string data;
    ReqData req_data;
    int req_index = 0;
    LOG(INFO) << "Context num: " << ctxs.size();
    for (int i = 0; i < (int)ctxs.size(); ++i) 
    {   
        // each tensor_array should add a type (correspond to the predictor type in strategy.conf of gi)
        // and a integer index (do not same)
        req_data.add_types(model_types_);
        req_data.add_indexes(req_index);
        req_index += 1;
        auto ta = req_data.add_tensor_arrays();
        CHECK_EQ(ctxs[i]->raw_images_size(), ctxs[i]->normalization_msg().data().pic_size());
        for (int j = 0; j < ctxs[i]->normalization_msg().data().pic_size(); ++j) 
        {
            if (ctxs[i]->normalization_msg().data().pic(j).failed_module().size() != 0 || ctxs[i]->raw_images(j).size() == 0) {
                fail_[i]++;
                continue;
            }
            auto t = ta->add_tensors();
            t->set_name("image");
            t->set_type("encoded_img");
            t->set_buffer(ctxs[i]->raw_images(j));
        }
    }
    req_data.SerializeToString(&data);

    PictureRequest req;
    // req.set_log_id(123456);
    req.set_log_id(rand() % 123456);
    req.set_app_id(appid_);
    req.set_data(data);

    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }
    brpc::Controller cntl;
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &resp_, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call model service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment() 
                   << ", status code: " << cntl.http_response().status_code() 
                   << ", remote side: " << cntl.remote_side();
        return false;
    }
    return true;
}

bool LabelControlSubModule::post_process(ContextPtr &ctx)
{
    std::vector<ContextPtr> batch{ctx};
    return post_process_batch(batch);
}

// enable rewrite.
bool LabelControlSubModule::post_process_batch(std::vector<ContextPtr> &ctxs)
{
    if (resp_.err_no() != 0) {
        LOG(ERROR) << mapped_model_keys_ << " service predect error: " << resp_.err_msg();
        add_err_num(ctxs.size());
        return false;
    }
    ResData res_data;
    auto result = resp_.result();
    if (!res_data.ParseFromString(result)) {
        LOG(ERROR) << mapped_model_keys_ << " result parse error: " << result;
        add_err_num(ctxs.size());
        return false;
    }

    // LOG(INFO) << "Context num: "<< ctxs.size() << ", tensor_arrays_size: " << res_data.tensor_arrays_size();
    CHECK_EQ(res_data.tensor_arrays_size(), (int)ctxs.size());
    if (res_data.tensor_arrays_size() != (int)ctxs.size()) {
        add_err_num(ctxs.size());
        LOG(ERROR) << "Context num: " << ctxs.size() << " not equal to "
                   << "tensor arrays size: " << res_data.tensor_arrays_size(); 
        return false;
    }
 
    for (int i = 0; i < res_data.tensor_arrays_size(); i++) {
        auto ta = res_data.tensor_arrays(i);
        // LOG(INFO) << "output type: " << ta.name();
        auto tensors = ta.mutable_tensors();
        int image_size = ctxs[i]->normalization_msg().data().pic_size() - fail_[i];  // True image num (in context) that are passed to model inference
        
        LOG(INFO) << "image size: " << image_size;
        auto tensor = tensors->Get(0);  // only one tensor in each tensor_array (when return score, concat operation is executed)
        int* shape_buf = tensor.mutable_shape()->mutable_data();
        std::vector<int> shape(shape_buf, shape_buf + tensor.shape_size());
        CHECK_EQ(image_size, shape[0]);  // 'tensor' represent one tensor, tensor shape is (N, k), N is image num (tensor num in request), k is cls num
        
        float* data_buf = tensor.mutable_data()->mutable_data();
        std::vector<float> scores(data_buf, data_buf + tensor.data_size());
        CHECK_EQ(image_size*class_num_, shape[0]*shape[1]);
        CHECK_EQ(image_size*class_num_, (int)scores.size());
        if (image_size*class_num_ != (int)scores.size()) {
            add_err_num(image_size*class_num_);
            LOG(ERROR) << "images size * class num before mapping: " << image_size*class_num_ << " not equal to "
                       << "score num before mapping: " << (int)scores.size();
            continue;
        }
        
        int tensor_count = 0;
        for (int j = 0; j < ctxs[i]->normalization_msg().data().pic_size(); ++j) {
            auto image = ctxs[i]->mutable_normalization_msg()->mutable_data()->mutable_pic(j);
            std::string country = ctxs[i]->normalization_msg().country();
            // LOG(INFO) << "country" << country;
            if (ctxs[i]->normalization_msg().data().pic(j).failed_module().size() != 0 || ctxs[i]->raw_images(j).size() == 0) {
                // image->mutable_detailmlresult()->insert(
                //         google::protobuf::MapPair<std::string, std::string>(ta.name(), MODEL_RESULT_FAIL));
                // the setting above for multi-label model
                image->mutable_detailmlresult()->insert(
                        google::protobuf::MapPair<std::string, std::string>(mapped_model_keys_, MODEL_RESULT_FAIL));
                continue;
            }
            
            int score_start_index = tensor_count*class_num_;
            tensor_count++;

            // aggragate the cls score each label
            std::vector<float> mapped_scores;
            for (int k = 0; k < (int)aggragation_types_list_.size(); ++k) {
                int mapped_index = k;
                float aggragation_score = 0.0;
                if (aggragation_types_list_[k] == "min") {
                    aggragation_score = 1.0;
                }
                for (int src_score_index = 0; src_score_index < class_num_; src_score_index++) {
                    if (mapping_indices_list_[src_score_index] == mapped_index) {
                        if (aggragation_types_list_[k] == "sum") {
                            aggragation_score += scores[score_start_index+src_score_index];
                        } else if (aggragation_types_list_[k] == "max") {
                            if (scores[score_start_index+src_score_index] > aggragation_score) {
                                aggragation_score = scores[score_start_index+src_score_index];
                            }
                        } else if (aggragation_types_list_[k] == "min") {
                            if (scores[score_start_index+src_score_index] < aggragation_score) {
                                aggragation_score = scores[score_start_index+src_score_index];
                            }
                        } else {
                            aggragation_score += scores[score_start_index+src_score_index];
                        }
                    }
                }
                mapped_scores.push_back(aggragation_score);
            }

            for (int k = 0; k < mapped_class_num_; ++k) {
                bool is_selected_index = false;
                for (int idx = 0; idx < (int)selected_indices_list_.size(); idx++) {
                    if (k == selected_indices_list_[idx]) {
                        is_selected_index = true;
                    }
                }
                if (!is_selected_index) {
                    continue;
                }

                // upload the threshold about each label
                // (*image->mutable_threshold())[mapped_model_keys_list_[k]] = threshold_manager_.get_threshold(country, mapped_model_keys_list_[k]);
                (*image->mutable_threshold())[mapped_model_keys_list_[k]] = threshold_manager_.get_threshold("default", mapped_model_keys_list_[k]);
                // upload push status
                // if (mapped_scores[k] > threshold_manager_.get_threshold(country, mapped_model_keys_list_[k])) {
                if (mapped_scores[k] > threshold_manager_.get_threshold("default", mapped_model_keys_list_[k])) {
                    image->mutable_detailmlresult()->insert(
                        google::protobuf::MapPair<std::string, std::string>(mapped_model_keys_list_[k], MODEL_RESULT_REVIEW));
                    add_hit_num(1);
                } else {
                    image->mutable_detailmlresult()->insert(
                        google::protobuf::MapPair<std::string, std::string>(mapped_model_keys_list_[k], MODEL_RESULT_PASS));
                }
                // upload the model score about each label
                // image->mutable_modelinfo()->insert(
                //     google::protobuf::MapPair<std::string, std::string>(mapped_model_keys_list_[k] + " score", std::to_string(mapped_scores[k])));
                // std::string model_res_str = std::string("{\"score\":[") + std::to_string(mapped_scores[k]) + std::string("]}");
                std::stringstream ss;
                ss << mapped_scores[k];
                std::string model_res_str = std::string("{\"score\":[") + ss.str() + std::string("]}");
                image->mutable_modelinfo()->insert(
                    google::protobuf::MapPair<std::string, std::string>(mapped_model_keys_list_[k], model_res_str));
            }
        }
    }
    return true;
}

RegisterClass(SubModule, LabelControlSubModule);
