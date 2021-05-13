#!/bin/sh
echo "Deployment k8s path: " $1


if [ ! -f "$1" ]; then
    echo "Not found file $1"
    exit -1
fi

namespace=`cat $1 | grep -m1 namespace | awk -F ':' '{print $2}' | awk '{print $1}'`
deployname=`cat $1 | grep -m1 name | awk -F ':' '{print $2}' | awk '{print $1}'`
conf_path=$(echo "$1" | sed "s/k8s/configmap/" | sed "s/.yaml/.conf/")
echo $conf_path

if [ -z "$namespace" ]; then
    echo "not found namespace"
    exit -1
fi
if [ -z "$deployname" ]; then
    echo "not found deployname"
    exit -1
fi

if [ ! -f "$conf_path" ]; then
    echo "Not found file $conf_path"
    exit -1
fi

echo "Find namespace : "  $namespace
echo "Find deployment: "  $deployname
echo "Find conf file : "  $conf_path

cat <<EOF >./kustomization.yaml
configMapGenerator:
- name: $deployname
  namespace: $namespace
  files:
  - $conf_path
generatorOptions:
  disableNameSuffixHash: true
EOF

kubectl apply -k ./