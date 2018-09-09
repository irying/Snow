

### master节点组件

### Node

Node 是 Pod 运行的地方，Kubernetes 支持 Docker、rkt 等容器 Runtime。 Node上运行的 Kubernetes 组件有 kubelet、kube-proxy 和 Pod 网络（例如 flannel）。



**kubelet**

kubelet 是 Node 的 agent，当 Scheduler 确定在某个 Node 上运行 Pod 后，**会将 Pod 的具体配置信息（image、volume 等）发送给该节点的 kubelet，kubelet 根据这些信息创建和运行容器，并向 Master 报告运行状态。**



**kube-proxy**

service 在逻辑上代表了后端的多个 Pod，外界通过 service 访问 Pod。service 接收到的请求是如何转发到 Pod 的呢？这就是 kube-proxy 要完成的工作。

每个 Node 都会运行 kube-proxy 服务，它负责将访问 service 的 TCP/UPD 数据流转发到后端的容器。如果有多个副本，kube-proxy 会实现负载均衡。



**Pod 网络**

Pod 要能够相互通信，Kubernetes Cluster 必须部署 Pod 网络，flannel 是其中一个可选方案。



**为什么 k8s-master 上也有 kubelet 和 kube-proxy 呢？**

**这是因为 Master 上也可以运行应用，即 Master 同时也是一个 Node。**



Kubernetes **通过各种 Controller 来管理 Pod 的生命周期**。为了满足不同业务场景，Kubernetes 开发了 Deployment、ReplicaSet、DaemonSet、StatefuleSet、Job 等多种 Controller。

###  Deployment

```
kubectl run httpd-app --image=httpd --replicas=2
```

Kubernetes 部署了 deployment `httpd-app`，有两个副本 Pod，分别运行在 `k8s-node1` 和 `k8s-node2`。

详细讨论整个部署过程。

① kubectl 发送部署请求到 API Server。

② API Server 通知 Controller Manager 创建一个 deployment 资源。

③ Scheduler 执行调度任务，将两个副本 Pod 分发到 k8s-node1 和 k8s-node2。

④ k8s-node1 和 k8s-node2 上的 kubelet在各自的节点上创建并运行 Pod。





总结一下这个过程：

1. 用户通过 `kubectl` 创建 Deployment。
2. Deployment 创建 ReplicaSet。
3. ReplicaSet 创建 Pod。



你只需要在Deployment中描述你想要的目标状态是什么，Deployment controller就会帮你将Pod和Replica Set的实际状态改变到你的目标状态。你可以定义一个全新的Deployment，也可以创建一个新的替换旧的Deployment。

一个典型的用例如下：

- 使用Deployment来创建ReplicaSet。ReplicaSet在后台创建pod。检查启动状态，看它是成功还是失败。
- **然后，通过更新Deployment的PodTemplateSpec字段来声明Pod的新状态。这会创建一个新的ReplicaSet，Deployment会按照控制的速率将pod从旧的ReplicaSet移动到新的ReplicaSet中。**
- 如果当前状态不稳定，回滚到之前的Deployment revision。每次回滚都会更新Deployment的revision。
- 扩容Deployment以满足更高的负载。
- **暂停Deployment来应用PodTemplateSpec的多个修复，然后恢复上线。**
- 根据Deployment 的状态判断上线是否hang住了。
- 清除旧的不必要的ReplicaSet。



##### 将pod调度到指定的node上

默认配置下，Scheduler 会将 Pod 调度到所有可用的 Node。不过有些情况我们希望将 Pod 部署到指定的 Node，**比如将有大量磁盘 I/O 的 Pod 部署到配置了 SSD 的 Node；或者 Pod 需要 GPU，需要运行在配置了 GPU 的节点上。**

**Kubernetes 是通过 label 来实现这个功能的。**

```
kubectl label node k8s-node1 disktype=ssd
kubectl get nodes
kubectl get node --show-labels
kubectl label node k8s-node1 disktype- 删除label
```



### DaemonSet

Deployment 部署的副本 Pod 会分布在各个 Node 上，每个 Node 都可能运行好几个副本。DaemonSet 的不同之处在于：每个 Node 上最多只能运行一个副本。

DaemonSet 的典型应用场景有：

1. 在集群的每个节点上运行存储 Daemon，比如 glusterd 或 ceph。
2. 在每个节点上运行日志收集 Daemon，比如 flunentd 或 logstash。
3. 在每个节点上运行监控 Daemon，比如 Prometheus Node Exporter 或 collectd。

其实 Kubernetes 自己就在用 DaemonSet 运行系统组件。执行如下命令：

```
kubectl get daemonset --namespace=kube-system
```

DaemonSet `kube-flannel-ds` 和 `kube-proxy` 分别负责在每个节点上运行 flannel 和 kube-proxy 组件。

[Kubernetes对象之ReplicaSet](https://www.jianshu.com/p/fd8d8d51741e)

> 在旧版本的Kubernetes中，只有ReplicationController对象。它的主要作用是**确保Pod以你指定的副本数运行**，即如果有容器异常退出，会自动创建新的 Pod 来替代；而异常多出来的容器也会自动回收。可以说，通过ReplicationController，Kubernetes实现了集群的高可用性。作者：cheergoivan链接：https://www.jianshu.com/p/fd8d8d51741e來源：简书简书著作权归作者所有，任何形式的转载都请联系作者获得授权并注明出处。
>
> 以上RS描述文件中，selector除了可以使用matchLabels，还支持集合式的操作：
>
> ```
>    matchExpressions:
>       - {key: tier, operator: In, values: [frontend]}
> ```



**controller会在下面3个业务场景中发挥作用，总结起来就是方便集群伸缩，达到高可用。**

```
假设有一个Pod正在提供线上服务，我们想想如何应对以下几个场景：

1.节日活动，网站访问量突增

2.遭到攻击，网站访问量突增

3.运行Pod的节点发生故障

第1种情况，活动前预先多启动几个Pod，活动结束后再结束掉多余的，虽然要启动和结束的Pod有点多，但也能有条不紊按计划进行。

第2种情况，正在睡觉突然手机响了说网站反应特慢卡得要死，赶紧爬起来边扩容边查找攻击模式、封IP等等……

第3种情况，正在休假突然手机又响了说网站上不去，赶紧打开电脑查看原因，启动新的Pod。
```



容器按照持续运行的时间可分为两类：**服务类容器和工作类容器**。

服务类容器通常持续提供服务，需要一直运行，比如 http server，daemon 等。工作类容器则是一次性任务，比如批处理程序，完成后容器就退出。

Kubernetes 的 Deployment、ReplicaSet 和 DaemonSet 都用于管理服务类容器；对于工作类容器，我们用 Job。**job又可以分为并行job，定时job。**



### Service

k8s是通过service来访问pod的，每个 Pod 都有自己的 IP 地址。当 controller 用新 Pod 替代发生故障的 Pod 时，新 Pod 会分配到新的 IP 地址。**service不管这些，servcie根据label来选择pod。**

```yaml
kind: Service
apiVersion: v1
metadata:
  name: search-svc
  namespace: search
spec:
  type: ClusterIP
  selector:
    app: search
  ports:
  - port: 8080
    protocol: TCP
    name: http
  - port: 9999
    protocol: TCP
    name: rpc
```

除了Cluster内部可以访问Service，很多情况下我们也希望应用的Service能够暴露给Cluster外部。**K8s提供了多种类型的Service，默认是ClusterIP。**

ClusterIP：Service通过Cluster内部的IP对外提供服务，只有Cluster内的节点和Pod可以访问

NodePort：<NodeIP>:<NodePort>

LoadBalaner：AWS等云服务商有它特有的loadbalaner将流量导过来。

```yaml
kind: Service
apiVersion: v1
metadata:
  name: search-svc
  namespace: search
spec:
  type: NodePort
  selector:
    app: search
  ports:
  - name: http
    port: 8888
    targetPort: 80
    nodePort: 30000
    protocol: TCP
```

nodePort是节点上监听的端口

port是ClusterIP上监听的端口

targetPort是Pod监听的端口



外部访问 curl 192.168.56.105:30000

内部会分配个ClusterIP，比如10.109.144.35，访问10.109.144.135:8888



**nodePort的工作原理与clusterIP大致相同，是发送到node上指定端口的数据，通过iptables重定向到kube-proxy对应的端口上。然后由kube-proxy进一步把数据发送到其中的一个pod上。**



### ConfigMap







## k8s和mesos的区别

mesos

开源的分布式资源管理框架，它被称为分布式系统的内核，**可以将不同的物理资源整合在一个逻辑资源层面上。当拥有很多的物理资源并想构建一个巨大的资源池的时候，mesos是最适用的。**可以看出mesos解决问题的核心是围绕物理资源层，上面跑什么，怎么跑它不关注，它缺少一个资源调度的东西。



k8s

自动化容器操作的平台，部署，调度和集群伸缩，它都有，有人研究在 mesos 系统上发布 k8s 作为 mesos 的调度器解决方案。



上面是从功能定位上的区别

业务类型也有区别

k8s支多种容器业务类型，一般有两种业务类，服务类跟工作类。服务类通常一直在运行，不能挂的，像web服务器，后台运行的daemon。工作类像批处理程序，定时任务这些。k8s提供job，可以设置并行参数。如果有计划任务，可以声明这个kind为cronJob。



### 工作流程到工作原理

https://www.yangcs.net/posts/what-happens-when-k8s/

首先通过 Deployment 部署应用程序，然后再使用 Service 为应用程序提供服务发现、负载均衡和外部路由的功能。



#### 内部服务发现—自发现

Service 提供了两种服务发现的方式，第一种是环境变量，第二种是 DNS。先说第一种，上面我们创建了`nginx-service`这个 Service，接着如果我们再创建另外一个 Pod，那么在这个 Pod 中，可以通过环境变量知道`nginx-service`的地址。



有个kube-dns组件，程监视 Kubernetes master 中的 **Service 和 Endpoint 的变化，并维护内存查找结构来服务DNS请求。**比如说在default的工作空间下新建了个nginx的service，集群内任何pod主要访问这个空间加上service名就能访问到。



APIserver收到Rest API请求后会进行一系列的验证操作，包括用户认证、授权和资源配额控制。

验证通过后，验证通过后，APIserver调用etcd的存储接口创建一个deployment controller对象。

controller manage里面的depolyment 类型controller会定期调用APIserver的API获取deployment（负责监听 Deployment 记录的更改），发现有新纪录了，那就开始创建replicatse纪录。

为什么不是创建pod纪录而是创建replicatset纪录呢。

在 K8s 中，Deployment 实际上只是一系列 `Replicaset` 的集合，而 Replicaset 是一系列 `Pod` 的集合。所以刚刚Deployment Controller 创建了第一个 ReplicaSet后，接着就是ReplicaSet Controller。



这个流程下来，etcd中就会保存一个deployment、一个replicaSet和pod资源纪录，这些pod资源现在还处于pending状态，因为它们还没被调度到合适的node中运行。



这时候就需要scheduler这个调度组件了。

Scheduler平时会使用APIserver的API，定期从etcd获取／监测系统中可用的工作节点列表和待调度的pod，并使用调度策略为pod选择一个运行的工作节点，这个过程叫做**绑定**。绑定成功后，scheduler会调用APIserver的API在etcd中创建一个binding对象，描述在一个工作节点上绑定运行的pod信息。

然后，节点上kubelet会监听APIserver上pod的更新，如果发现有pod更新，则会自动创建并启动。



Kube-proxy



