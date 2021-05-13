## 简介

likee短视频 流量控制层， 可以在此处实现一些模型的综合策略

红线提醒: 别在自己的分支点deploy，会直接覆盖线上的程序，建议开发的时候，直接把`.gitlab-ci.yaml`最后面的deploy-jja; deploy-eu这些都删了

## 主流程 

​	整体程序就是一个消费者+生产者的串联Pipeline

​	通过一个所用模块共用的Context来传递信息



1. 从kafka获取message，并解析

   - 对应代码 `kafka_client.cpp`

   - 具体Message格式可见`util.proto:KafkaConsumer`, 字段含义可参考[此文档](http://wiki.bigo.sg:8090/pages/viewpage.action?pageId=164856783) 

      wiki文档实质是前机审的YY协议格式，目前对字段位置有稍微修改，但字段含义未变

   

2. 下载视频和封面

    - 对应代码 `kafka_client.cpp`

    - 根据视频URL，封面URL下载对应数据，并将对应的下载内容转发给各个模型

    

3. 请求对应的模型: likecore/ocr/ann

    - 对应代码 `dispacher.cc` 与 `src/submodule`

    - dispacher模块是submodule的调用模块，新增模型应该是新增一个对应的submodule

    

4. 调用人审接口(无论高危都需要调用)

   - 对应代码`review_callback_v2.cpp`

   - 人审接口定义可以参考[此文档](http://wiki.bigo.sg:8090/pages/viewpage.action?pageId=155551396)

   

5. 上报hive

    - 对应代码`uploader_hive.cpp`

    -  可以使用[百纳接口](http://wiki.bigo.sg:8090/pages/viewpage.action?pageId=131335047), 目前我们使用纯JSON格式进行上报

      

### Kafka consumer

- 简介

  Kafka client端主要工作就是从like实时流的kafka获取对应的视频meta信息，并进行相应的解析

  目前主要因为机审跟随视频存储一起部署，目前划分为3个大区，对应为HK/EU/AM

  不同区域的数据会存储在不同的TOPIC内，每个大区拉取各自的Topic进行消费

- 注意事项

  1. 生产环境使用大数据的rec-kafka集群，这个集群部署在HK

     EU/AM的程序是需要到HK的机器来拉取kafka信息的，不过存入kafka的meta信息，每条大小也就几Kb，一般情况下都不会有什么问题

     

  2. Restart保护机制

     目前我们采用的Auto Commit, 而我们的整个程序是多个生产者/消费者组成的糖葫芦，每个生产者和消费者都有一个Buffer Queue进行连接，如果程序重启，这些Queue里面的数据都未实际过审，而kafka已经接受到了commit，如果下次直接启动，那么这部分buffer数据就会漏审

     对此，在每次client启动的时候，检测到kafka rebalance(即有groupid内的consumer个数发生变化)，每个consumer会对当前partition offset回滚512条message

     目前每个Topic有6个Partition，每次restart，程序实际回滚 6 * 512条message，尽量减少restart导致的数据丢失问题

     这儿带来的一个问题就是，人审接口会重复接受到一条消息的推送，一条消息在Hive表中也会存在多个结果，另外可能就是新产生3000来条数据，如果模型服务本来负载处于峰值，可能就处理不过来了

     人审接口目前是有去重机制的，这个解决了第一个问题

     Hive表这个没有办法避免，虽然我们也可以做一个去重机制，但是这样会加大项目复杂度，也不会带来太大收益，大家查询Hive的时候注意此点即可

     模型处理处理压力过大，本质上是一个资源问题，此点也没有什么特别好的解决办法，只能说申请资源的时候，需要有一定的冗余空间
     
3. Lag监控
  
   目前lag监控是直接加在client端代码，这儿会有一些缺陷，同时也加入了大数据的Lag监控机制, 此处监控文档待后续补充
  
   线上的消费情况在这儿也可以看
  
   - [欧洲](http://139.5.108.236:8080/clusters/kafka_rec/consumers/ml_like_video_audit_eu_online/topic/ml_like_video_audit_eu/type/KF)
  
   - [香港](http://139.5.108.236:8080/clusters/kafka_rec/consumers/ml_like_video_audit_hk_online/topic/ml_like_video_audit_hk/type/KF)
  
   - [美洲](http://139.5.108.236:8080/clusters/kafka_rec/consumers/ml_like_video_audit_am_online/topic/ml_like_video_audit_am/type/KF)
  
   

### Downloader

- 简介

  视频/封面下载器

- 注意事项

  1. 失败机制

     上线的时候CDN老抽风，失败的概率高于正常值。目前对封面/视频采用了独立的重试机制，每个message重试X次（默认为4），每次失败后会sleep两秒，一般失败后直接去试是再次失败，加入sleep机制成功率会稍微高一点，也可以避免对CDN造成较大的流量冲击

     另外只有视频下载失败，这条message才会失败，如果只是封面下载失败，那这个错误将会被忽略

     目前下载视频下载失败后，不会继续下载图片，但是不会丢弃这条信息，会继续向下传，在dispatcher模块，各个submodule在请求前会检查downloader content内是否存在视频内容/封面内容，如果内容为空，就返回失败

     最后人审接口/hive会接受到这条机审失败的信息, 避免进入超时漏审流程

  2. 限流问题

     CDN好像对单个IP有限流，单机网卡有限，同时出于容灾的策略，部署时尽量做到每个大区3机房等量

     每个Topic都有6个Partition，因此正好每个机房最大两个实例

  3. 解码集群下载器
    
     解码集群下载器不会发送bitmap给各个模型服务，各个模型服务自己请求解码集群即可。解码集群下载器只是起到一个热身作用，令各个模型服务可以命中解码集群的CDN缓存

     使用解码集群下载器，也不会再提供封面下载功能
     

### Dispatcher

- 简介

  请求分发在此进行

  此处有个submodule基类，每个模型的client的大部分逻辑应该在此实现

  dispatcher会并行同时请求每个模型

- 注意事项

  1. 多线程问题

     因为目前的设计特别简单暴力，一个Context从头传到尾，submodule都会涉及到修改同一个Context内的`ReviewCallback`字段，告诉人审自己模型的结果，但是多个`submodule`都是并发执行的，同时修改一个结构体内的一个字段时候，这个行为就是未定义的，程序一下就会coredump给你看

     因此submodule这个模块分成了两个部分， `call_service`和`post_process`

     `call_service`函数用于执行所有的网络请求，和非临界区代码

     `post_process`函数用于执行临界区代码，整个函数都使用了一把大锁进行保护，可以在此修改`ReviewCallback`等公共字段，应尽量避免在此处调用一些耗时的网络逻辑

  2. 重试问题

     首先明确一个原则，重试应当保守。

     - 是否重试？重试搞不好就会造成雪崩，把模型服务打挂。但是重试能够提高模型的成功率，另外这个模型的平均响应时间是多少？会不会因为这个模型的大量重试而阻塞整条Pipeline

     - 如何重试？尽量休眠一段时候后再进行重试，重试的时候应该有LOG记录，达到最大重试次数之后应该有丢弃告警

  3. 失败处理

     考虑一个问题，如何定义机审失败。机审失败的结果是直接推送至人审

     一般来说，我们不能因为单个模型的失败而视为机审失败

     目前是定义`PornSoftporn`模型为核心模型，此模型失败后会有重试，重试之后失败会视为机审失败

     不过如果其他模型认为此视频为高危视频，`PornSoftporn`调用失败，那我们也试为机审成功

     机审失败的直接体现是`ReviewCallback`的`resultcode`为0

  

### Submodule

- 简介

  实际模型服务的client端，大家尽情发挥

  注意一个原则，不要因为一个模型的失败处理存在问题，或者重试机制存在问题，而导致整个like审核流程不可用，当然submodule这块的风险相对小一些

  

### Controller

- 简介

  不用找了，目前没有这个代码

  整个机审的综合逻辑控制应该在拿到各个模型结果之后，再进行一些后处理操作，当然也可以再次去过其他模型，只需要集成Module模块，并将新Module的name传递给`enable_modules`参数

  这块的相当于submodule的Join/Merge模块，风险较大，请谨慎处理逻辑

  

### Uploader

- 简介

  结果上报（人审回调/Hive写入/Kafka写入/DB写入）

- 注意事项

  - 如果新增一个模型，需要将结果发送给人审，需要和人审方商议具体协议，具体可以找对接@全超豪@洪庆春@姚星辉
  - 模型服务如果能够直接上报，就直接在模型服务里面上报，别走这儿了（本来这块中控代码就是一个大杂烩，修改此处代码风险远远大于修改单个模型服务的风险）





## 上线

### 灰度测试

上线前需要到灰度环境里面走一走，会有测试同学在灰度测试整套流程的逻辑正确性，链路正确性

因为我们的灰度环境，是手动在like内测客户端灰度环境发送一条视频，灰度才会有一条数据，灰度每天就几百来条数据，测测逻辑还行，程序正确性(极端QPS下会不会挂掉，特殊视频会不会导致程序挂掉)，还得采取其他方式



### AB测试

此处就已经到线上部署了，也就是逐步放量，后续会提供细致的流量控制方式，目前只能采取大区逐步放量的手段来切分流量

正常来讲首先在AM大区部署，AM大区也占全球10%的量

然后增加EU，AM+EU的流量差不多占全球50%了

最后增加HK

中途出现任何问题，直接回滚


### 正式上线

应该关心下面这几个报表

[APM](https://statistics.bigo.tv/stats/index.html#/GeneralReport/7)

报表中心-秩序业务指标

报表中心-秩序机审指标



## 兜底策略

1. 超时漏审策略

   人审会扫描like新增视频的状态，新增视频超过5Min没有过机审将会直接送入人审

   APM有一张机审超时漏审报表，此策略主要用于处理有些视频无法下载/无法解码的情况

   如果Pipeline发生阻塞或者出现故障，会有大量超时视频被直接送入人审

   对于这种情况，我们加入了kafka lag监控来进行消费情况监测。对于异常状况，可以和人审同学进行沟通，将一定时间段的视频重新过机审，重新推送给人审，人审会将重推的视频从人审队列中去除

   

2. 容灾策略

   供3个大区，每个大区共有3个机房，核心模型的程序应该平均分配在每个机房，50%的资源冗余，即可以容忍任意一个机房宕机


### 监控告警相关

利用prometheus metrics 做监控告警需要了解metrics 和 promQL 相关概念,  可以参考

[Prometheus 官方文档](https://prometheus.io/)

[详细使用教程](https://yunlzheng.gitbook.io/prometheus-book/)


Kafka Consumer 的metrics 已接入公司监控中，可自定义metrics 通过组合进行监控告警, 如推送比控制。监控告警等功能由Alert 平台提供, 需要咨询问题的同学可加入 Alert 平台用户群。

[Alert 平台 prometheus 查询入口](http://alert.bigo.sg/promxy/graph)

[Alert 平台 grafana入口](http://metrics-sysop.bigo.sg/)

[Alert 平台 告警配置中心](http://alert.bigo.sg/)

[Alert 平台WIKI](https://git.sysop.bigo.sg/dev-tools/working-with-metrics/wikis/pages)

短视频机审报表地址 : http://metrics-sysop.bigo.sg/d/_uNrsIzGz/duan-shi-pin-ji-shen-jian-kong?orgId=1 

主视图右上角为短视频机审Lag监控， ml_like_audit_(hk | eu | am) 分别对应香港， 欧洲， 美洲的kafka 积压，其余为各类旁路模型的kafka 积压。


需要规范 bvar 的命名以便自动生成报表，最终bvar / metrics 的分隔符会被brpc统一用 '_' 替换掉。
bvar 的使用方法可参考这两个文档：

https://github.com/apache/incubator-brpc/blob/master/docs/cn/bvar.md

https://github.com/apache/incubator-brpc/blob/master/docs/cn/bvar_c++.md#quick-introduction


对于Module:

- 用于队列积压统计：

  ~~~c++
  "bigoai.$(module).queue.size.avg.10s"
  ~~~

- 用于错误统计:

  ~~~c++
  # 错误总数
  "bigoai.$(module).error.count"
  
  # 错误eps
  "bigoai.$(module).error.qps"
  ~~~

- 用于时延统计：

  ~~~c++
  "bigoai.$(module)"
  ~~~

  

对于submodule, 以封禁库为例：

~~~c++
bvar::LatencyRecorder g_blacklist("bigoai.model", "blacklist");
bvar::Adder<int> g_blacklist_risk("bigoai.model", "blacklist.risk.count");
bvar::Adder<int> g_blacklist_req("bigoai.model", "blacklist.req.count");
bvar::Adder<int> g_blacklist_error("bigoai.model", "blacklist.error");

~~~

- 用于时延统计 : 

  ~~~c++
  bvar::LatencyRecorder g_${submodule}("bigoai.model", "$(submodule)");
	~~~
	
- 用于推送比：

  ~~~c++
  # 子模型高危请求计数
  bvar::Adder<int> g_${submodule)_risk("bigoai.model", "${submodule).risk.count"); 
  
  # 子模型请求总数
  bvar::Adder<int> g_${submodule)_req("bigoai.model", "${submodule).req.count");
  ~~~
  
- 用于调用错误统计 ：

  ~~~c++
  bvar::Adder<int> g_${submodule)_req("bigoai.model", "${submodule).error");
  ~~~

- 对于新增的bvar，建议命名为 ( 注意不是bvar 变量的命名, 而是生成在bvar 界面中显示的名字)  

  ~~~C++
  bigoai.model.$(submodel).$(bvar属性)
  ~~~

  最终会被规范化为

  ~~~C++
  bigoai_model_$(submodel)_$(bvar属性)
  ~~~

   具体的查询语句实例可点击对应机审报表，点击edit 查看PromQL 语句。

## 其他

- 旁路模型

  旁路模型，指结果不需要上报人审，但同时需要like实时流数据的模型，例如，用于欧美未成年人打散策略的年龄模型和like视频去重模型

  旁路模型不要和like审核主流程整合在一起，相应来说旁路模型不需要太强的时效性，可以采用offline的方式(计划提供相应模板)，或者单独部署client的方式进行处理，只需要消费线上的kafka数据即可

  

- 相关项目
  - [kafka-consumer-python](https://git.sysop.bigo.sg/infer-serving/kafka-consumer)
  - [kafka-consumer-go](https://git.sysop.bigo.sg/infer-serving/kafka-consumer-go)



## 本地测试

1. 在项目的根目录实现local.conf，本地测试默认使用此配置文件（在Dockerfile.CMD.config_file指定）

2. 必须确保local.conf 的Kafka相关的groupid与其他测试者不同，最好使用自己的名字作为groupid
3. 必须关闭review(人审)/duplicated(去重)/upload(Hive上报)等模块，避免数据污染，如果测试数据被回调至线上，将导致生产事故
4. 禁止直接复制生产配置为local.conf，危险操作!

```
编译:  sudo docker build ./ -t kafka
运行:  sudo docker run -rm --name kafka kafka
停止:  sudo docker stop kafka
退出:  sudo docker kill kafka
```



## FQA



