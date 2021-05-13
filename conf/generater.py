import os
import sys
import yaml
import copy
import value

def find_pos(container_list, container_name):
  for i in range(len(container_list)):
    if container_list[i]["name"] == container_name:
      return i
  raise Exception("Invalid container_name !", container_name)
  
def generate_grey():

  filename = "template/deploy.yaml"

  with open(filename, "r") as fp, open("grey-deploy.yaml", "w") as fo:
    conf = yaml.load(fp) 
    conf["metadata"]["name"] = value.service_name
    conf["metadata"]["namespace"] = "grey" 
  
    conf["spec"]["replicas"] = 1 
    conf["spec"]["template"]["metadata"]["labels"]["app"] = value.service_name
    conf["spec"]["selector"]["matchLabels"]["app"] = value.service_name
  
    spec = conf["spec"]["template"]["spec"]
    spec["affinity"]["nodeAffinity"]["requiredDuringSchedulingIgnoredDuringExecution"]["nodeSelectorTerms"][0]["matchExpressions"][0]["key"] = "grey"
    
    # gen instance
    spec["containers"][0]["name"] = value.service_name

    spec["containers"][0]["resources"]["limits"]["memory"] = value.memory
    spec["containers"][0]["resources"]["limits"]["cpu"] = value.cpu    

    # gen sidecar
    sidecar_pos = find_pos(spec["containers"], "brpc-portal-sidecar")
    if value.use_decode_sidecar:
      spec["containers"][sidecar_pos]["command"].append("-forward_urls=/api/{}".format(value.service_name)) 
      spec["containers"][sidecar_pos]["command"].append("-service_name={}".format(value.service_name))
    
    fo.write(yaml.dump(conf))
  
  
  with open("template/route.yaml", "r") as fp:
    conf = [i for i in yaml.load_all(fp)]
    service = conf[0]
    vservice = conf[1]
    destination = conf[2]
  
    service["metadata"]["name"] = value.service_name
    service["metadata"]["namespace"] = "grey" 
    service["metadata"]["labels"]["app"] = value.service_name
    service["spec"]["selector"]["app"] = value.service_name
    if value.use_decode_sidecar:
      service["spec"]["ports"][0]["targetPort"] = 8000
    with open("grey-service.yaml", "w") as fo:
      fo.write(yaml.dump(service))
  
    vservice["metadata"]["name"] = value.service_name
    vservice["metadata"]["namespace"] = "grey" 
    vservice["spec"]["http"][0]["match"][0]["uri"]["prefix"] = "/grey/{}".format(value.service_name)
    vservice["spec"]["http"][0]["rewrite"]["uri"] = "/api/{}".format(value.service_name)
    vservice["spec"]["http"][0]["route"][0]["destination"]["host"] = value.service_name
    with open("grey-virtualService.yaml", "w") as fo:
      fo.write(yaml.dump(vservice))
  
    destination["metadata"]["name"] = value.service_name
    destination["metadata"]["namespace"] = "grey" 
    destination["spec"]["host"] = value.service_name
    with open("grey-destinationRule.yaml", "w") as fo:
      fo.write(yaml.dump(destination))

def generate_production():
  filename = "template/deploy.yaml"
  
  with open(filename, "r") as fp, open("deploy-hk.yaml", "w") as fo_hk, open("deploy-eu.yaml", "w") as fo_eu, open("deploy-cn.yaml", "w") as fo_cn, open("deploy-sg.yaml", "w") as fo_sg :
    conf = yaml.load(fp) 
    conf["metadata"]["name"] = value.service_name
    conf["metadata"]["namespace"] = value.username
  
    #conf["spec"]["replicas"] = value.replicas
    conf["spec"]["template"]["metadata"]["labels"]["app"] = value.service_name
    conf["spec"]["selector"]["matchLabels"]["app"] = value.service_name
  
    spec = conf["spec"]["template"]["spec"]
    spec["affinity"]["nodeAffinity"]["requiredDuringSchedulingIgnoredDuringExecution"]["nodeSelectorTerms"][0]["matchExpressions"][0]["key"] = value.bucketname
    spec["affinity"]["podAntiAffinity"]["requiredDuringSchedulingIgnoredDuringExecution"][0]["labelSelector"]["matchExpressions"][0]["values"][0] = value.service_name
  
    # gen instance
    spec["containers"][0]["name"] = value.service_name
    spec["containers"][0]["resources"]["limits"]["memory"] = value.memory
    spec["containers"][0]["resources"]["limits"]["cpu"] = value.cpu

    #gen sidecar
    sidecar_pos = find_pos(spec["containers"], "brpc-portal-sidecar")
    if value.use_decode_sidecar:
      spec["containers"][sidecar_pos]["command"].append("-forward_urls=/api/{}".format(value.service_name)) 
      spec["containers"][sidecar_pos]["command"].append("-service_name={}".format(value.service_name)) 
    else:
      spec["containers"].pop(sidecar_pos)

    spec["containers"][0]["livenessProbe"]["initialDelaySeconds"] = value.init_delay_second
    spec["containers"][0]["readinessProbe"]["initialDelaySeconds"] = value.init_delay_second

    conf["spec"]["replicas"] = value.hk_replicas
    fo_hk.write(yaml.dump(conf))
    conf["spec"]["replicas"] = value.eu_replicas
    fo_eu.write(yaml.dump(conf))
    conf["spec"]["replicas"] = value.cn_replicas
    fo_cn.write(yaml.dump(conf))
    conf["spec"]["replicas"] = value.sg_replicas
    fo_sg.write(yaml.dump(conf))

    if value.enable_hpa:
      fo_hk.write("---\n")
      fo_eu.write("---\n")
      fo_cn.write("---\n")
      fo_sg.write("---\n")
      with open("template/hpa.yaml", "r") as fp:
        hpa = yaml.load(fp)
        hpa["metadata"]["name"] = value.service_name
        hpa["metadata"]["namespace"] = value.username
        hpa["spec"]["scaleTargetRef"]["name"] = value.service_name
        hpa["spec"]["metrics"][0]["resource"]["target"]["averageUtilization"] = value.cpu_average_utilization

        hpa["spec"]["minReplicas"] = value.hk_min_replicas
        hpa["spec"]["maxReplicas"] = value.hk_max_replicas
        fo_hk.write(yaml.dump(hpa))
        hpa["spec"]["minReplicas"] = value.eu_min_replicas
        hpa["spec"]["maxReplicas"] = value.eu_max_replicas
        fo_eu.write(yaml.dump(hpa))
        hpa["spec"]["minReplicas"] = value.cn_min_replicas
        hpa["spec"]["maxReplicas"] = value.cn_max_replicas
        fo_cn.write(yaml.dump(hpa))
        hpa["spec"]["minReplicas"] = value.sg_min_replicas
        hpa["spec"]["maxReplicas"] = value.sg_max_replicas
        fo_sg.write(yaml.dump(hpa))

  with open("template/route.yaml", "r") as fp:
    conf = [i for i in yaml.load_all(fp)]
    service = conf[0]
    vservice = conf[1]
    destination = conf[2]
  
    service["metadata"]["name"] = value.service_name
    service["metadata"]["namespace"] = value.username
    service["metadata"]["labels"]["app"] = value.service_name
    service["spec"]["selector"]["app"] = value.service_name
    if value.use_decode_sidecar:
      service["spec"]["ports"][0]["targetPort"] = 8000
    with open("service.yaml", "w") as fo:
      fo.write(yaml.dump(service))
  
    vservice["metadata"]["name"] = value.service_name
    vservice["metadata"]["namespace"] = value.username
    vservice["spec"]["http"][0]["match"][0]["uri"]["prefix"] = "/api/{0}/{1}".format(value.username, value.service_name)
    vservice["spec"]["http"][0]["rewrite"]["uri"] = "/api/{}".format(value.service_name)
    vservice["spec"]["http"][0]["route"][0]["destination"]["host"] = value.service_name
    with open("virtualService.yaml", "w") as fo:
      fo.write(yaml.dump(vservice))
  
    destination["metadata"]["name"] = value.service_name
    destination["metadata"]["namespace"] = value.username
    destination["spec"]["host"] = value.service_name
    with open("destinationRule.yaml", "w") as fo:
      fo.write(yaml.dump(destination))

  with open("template/monitor.yaml", "r") as fp, open("monitor.yaml", "w") as fo:
    conf = yaml.load(fp) 
    conf["metadata"]["name"] = value.service_name
    conf["metadata"]["namespace"] = value.username
    conf["metadata"]["labels"]["app"] = value.service_name
    
    conf["spec"]["jobLabel"] = value.service_name
    conf["spec"]["namespaceSelector"]["matchNames"][0] = value.username
    conf["spec"]["selector"]["matchLabels"]["app"] = value.service_name
    fo.write(yaml.dump(conf))
    

def generate_ab():

  filename = "template/deploy.yaml"

  with open(filename, "r") as fp, open("ab-deploy.yaml", "w") as fo:
    conf = yaml.load(fp) 
    conf["metadata"]["name"] = value.service_name
    conf["metadata"]["namespace"] = "ab-test"
  
    conf["spec"]["replicas"] = value.ab_replicas 
    conf["spec"]["template"]["metadata"]["labels"]["app"] = value.service_name
    conf["spec"]["selector"]["matchLabels"]["app"] = value.service_name
  
    spec = conf["spec"]["template"]["spec"]
    spec["affinity"]["nodeAffinity"]["requiredDuringSchedulingIgnoredDuringExecution"]["nodeSelectorTerms"][0]["matchExpressions"][0]["key"] = "ab-test"
  
    # gen instance
    spec["containers"][0]["name"] = value.service_name
    spec["containers"][0]["resources"]["limits"]["memory"] = value.memory
    spec["containers"][0]["resources"]["limits"]["cpu"] = value.cpu

    #gen sidecar
    sidecar_pos = find_pos(spec["containers"], "brpc-portal-sidecar")
    if value.use_decode_sidecar:
      spec["containers"][sidecar_pos]["command"].append("-forward_urls=/api/{}".format(value.service_name)) 
      spec["containers"][sidecar_pos]["command"].append("-service_name={}".format(value.service_name)) 
    else:
      spec["containers"].pop(sidecar_pos)

    fo.write(yaml.dump(conf))


  with open("template/route.yaml", "r") as fp:
    conf = [i for i in yaml.load_all(fp)]
    service = conf[0]
    vservice = conf[1]
    destination = conf[2]
  
    service["metadata"]["name"] = value.service_name
    service["metadata"]["namespace"] = "ab-test"
    service["metadata"]["labels"]["app"] = value.service_name
    service["spec"]["selector"]["app"] = value.service_name
    if value.use_decode_sidecar:
      service["spec"]["ports"][0]["targetPort"] = 8000
    with open("ab-service.yaml", "w") as fo:
      fo.write(yaml.dump(service))
  
    vservice["metadata"]["name"] = value.service_name
    vservice["metadata"]["namespace"] = "ab-test"
    vservice["spec"]["http"][0]["match"][0]["uri"]["prefix"] = "/api/ab-test/{0}".format(value.service_name)
    vservice["spec"]["http"][0]["rewrite"]["uri"] = "/api/{}".format(value.service_name)
    vservice["spec"]["http"][0]["route"][0]["destination"]["host"] = value.service_name
    with open("ab-virtualService.yaml", "w") as fo:
      fo.write(yaml.dump(vservice))
  
    destination["metadata"]["name"] = value.service_name
    destination["metadata"]["namespace"] = "ab-test"
    destination["spec"]["host"] = value.service_name
    with open("ab-destinationRule.yaml", "w") as fo:
      fo.write(yaml.dump(destination))

generate_grey()
generate_production()
generate_ab()

# TODO: Modify kafka 
#
#TODO: Modify monitor
