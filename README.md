# QuickHTTP
C++ Parallel Computing and Asynchronous Networking Engine

主要参考：https://github.com/sogou/workflow

#### 系统设计特点
普通开发工作，一个后端进程由三个部分组成，并且完全独立开发。即：程序 = 协议 + 算法 + 任务流
- 协议
	大多数情况下，用户使用的是内置的通用网络协议，例如http，redis,或者各种rpc
	用户可以自己定义网络协议，只需要提供序列化和反序列化函数，就可以定义出自己的client/server
- 算法
	在我们的设计里面，算法是与协议对称的概念
		如果说协议的调用是rpc，算法的调用就是一次apc(Async Procedure Call)
	我们提供了一些通用算法，例如sort,merge,psort,reduce,可以直接使用
	与自定义的协议相比较，自定义的算法要常见的多。任何一次边界清晰的复杂计算，都应该包装成算法。
- 任务流
	任务流就是实际的业务逻辑，就是把开发好的协议与算法放在流程图里面使用起来
	典型的任务流是一个闭合的串并联图。复杂的业务逻辑，可能是一个非闭合的DAG

异步性和基于C++11 std::function的封装
- 不是基于用户态协程。使用者需要知道自己在写异步程序
- 一切调用都是异步执行，几乎不存在占着线程等待的操作
  虽然提供一些便利的半同步接口，但并不是核心的功能
- 尽量避免派生，以std::function封装用户行为，包括：
	任何任务的callback
	任何server的process。符合FaaS(Function as a Service)思想
	一个算法的实现，简单来讲也是一个std::function。但算法可以用派生实现
