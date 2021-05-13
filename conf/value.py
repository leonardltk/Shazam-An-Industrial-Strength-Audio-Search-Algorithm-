service_name = "hello-shash-serv"
username = "hello"
#bucketname = "buffer"  # 资源池名称；后续资源分配按照资源池；各个业务线/team 使用各自的资源池，用于业务成本核算，也可增加部署的灵活性
bucketname = "audio"  # 资源池名称；后续资源分配按照资源池；各个业务线/team 使用各自的资源池，用于业务成本核算，也可增加部署的灵活性

# K8S config
cpu = 16
memory = "24Gi"
gpu_memory = 4
ab_replicas = 0
hk_replicas = 0
eu_replicas = 0
cn_replicas = 0
sg_replicas = 1
init_delay_second = 15

#HPA 
enable_hpa = True
hk_max_replicas = 3 
hk_min_replicas = 1
eu_max_replicas = 3 
eu_min_replicas = 1
cn_max_replicas = 3 
cn_min_replicas = 1
sg_max_replicas = 1
sg_min_replicas = 1
cpu_average_utilization = 50

ins_num_per_gpu = 0
bmls_ret_type = "VideoTag"

# decode sidecar config
use_decode_sidecar = False

# srv config
use_srv = False
host_port = 19999
