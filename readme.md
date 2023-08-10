# taskflow

首先我们找到不依赖任何task的task集合，全部抛到线程池；之后每个task执行完毕之后，会回复给主控一个信号；收到信号之后，我们就通知所有依赖这个task的task，告诉它这个task执行完了，每个task都记录着一个数字，标记它依赖的task有多少还没有执行完（初始和它依赖的task数相同），当收到通知后就-1；当这个数字为0时，则把这个task也抛到线程池。 最后直到所有task执行完，结束。

并且我们也很容易实现taskflow中的子task逻辑；实现方法为在task中再存一个children数组，则每次在把task抛到线程池之前，先检查有没有子task(也就是是否为叶子节点），如果是直接抛，否则；从子task中找出没有依赖的task;抛到线程池。

同时每个task还需要一个父节点；当这个task执行完毕之后，就通知父节点，说它执行完了。而每个task还需维护一个数字，表示它的直接子task有多少没有执行完，当这个数字为0时；表示它的所有子task都执行完了；则可以把这个task本身抛到线程池。

# coding

## 引用

```c++
size_t Task::getTaskNum() {
        size_t res = 0;
        for (auto& it : children_) {
            // 引用对象，不进行拷贝，高效
            res += it -> getTaskNum();
        }
        res += 1; // self
        return res;
    }
```

在这段代码中，`auto& it` 使用的是引用类型，而不是普通的值类型。这是因为在循环中遍历容器的元素时，使用引用类型可以带来以下几个好处：

1. **避免拷贝：** 使用引用类型可以避免将容器中的每个元素都拷贝到一个新的变量中，从而节省内存和时间开销，特别是在处理大型容器时。

2. **允许修改：** 如果循环体内需要对遍历的元素进行修改，使用引用类型可以直接修改原始容器中的元素，而不是操作它们的副本。这可以确保修改的内容在循环结束后在容器中保留。

3. **更高效：** 引用类型避免了指针操作，因此在某些情况下可能会比使用普通值类型更高效。

因此，在这段代码中使用 `auto& it` 是为了遍历容器中的每个元素而不进行拷贝，同时还允许对元素进行修改和操作。

## wait

```c++
void MsgQueue::wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return (!que_.empty());
        });
    }
```

这段代码是使用`std::condition_variable`的`wait()`函数来实现线程等待，直到某个条件成立。

1. `condition_` 是一个`std::condition_variable`的实例，用于在线程间同步和通信。

2. `lock` 是一个`std::unique_lock<std::mutex>`，它持有一个互斥锁（mutex）。在调用`wait()`之前，你需要锁定这个互斥锁，以防止多个线程同时操作共享资源。

3. `[this]()` 是一个Lambda函数，它是`wait()`函数的第二个参数。它定义了等待条件，只有当这个Lambda函数返回`true`时，线程才会被唤醒。

4. `return (!que_.empty())` 是Lambda函数的返回值，它表示等待条件。在这个例子中，线程会等待直到`que_`不为空（即队列中有数据）。

所以，这段代码的作用是：线程会在`condition_`上等待，直到队列`que_`中有数据。一旦队列中有数据，等待条件就会成立，线程会被唤醒并开始执行后续操作。通常这种用法用于生产者-消费者模式，其中消费者等待生产者将数据放入队列中，一旦有数据可用，消费者就会被唤醒来处理数据。

## creatThread

```c++
inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // 会在当前状态阻塞，知道条件成立，stop 线程池停止或者任务队列为空
                    this -> condtion.wait(lock,
                                        [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty()) {
                        return;
                    }
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}
```

这段代码是线程池的构造函数实现，其中的 `ThreadPool` 类会在初始化时创建指定数量的线程，并让这些线程循环地从任务队列中获取任务并执行。

1. 在线程池的构造函数中，你传入一个 `threads` 参数，它指定了线程池中要创建的线程数量。

2. 使用 `workers.emplace_back([this] { ... })` 循环创建指定数量的线程，每个线程都执行一个 Lambda 函数。

3. 在每个线程的循环中，使用 `std::unique_lock<std::mutex> lock(this->queue_mutex)` 来获取互斥锁，以确保线程安全地访问任务队列。

4. 使用 `this->condtion.wait(lock, [this] { ... })` 来等待条件变量，直到有任务可执行或者线程池被停止。

5. 如果线程池被停止，并且任务队列为空，则线程退出。

6. 如果有任务可执行，从任务队列中取出任务，执行它，并继续下一个循环。

```cpp
this->condtion.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
```

1. `this->condtion.wait(lock, ...)`：这是条件变量的等待操作，它会在当前线程上阻塞，直到条件满足。它需要一个互斥锁 `lock` 来确保线程安全。等待期间，会释放这个互斥锁，以便其他线程可以操作。

2. `[this] { return this->stop || !this->tasks.empty(); }`：这是等待的条件，是一个 Lambda 函数。在等待期间，线程会检查这个条件是否为真。条件满足的情况是：要么 `stop` 标志为真（表示线程池停止），要么任务队列 `tasks` 非空。
- 如果 `stop` 为真，表示线程池被要求停止，那么即使任务队列不为空，线程也应该停止等待，因为线程池已经被停止了。
    
- 如果 `tasks` 非空，表示任务队列中有任务需要执行，线程应该立即唤醒，执行任务。

这个等待条件的目的是在以下情况下进行等待：

- 当没有任务可执行且线程池还未被停止时，线程会等待，直到有任务加入任务队列或者线程池被停止。

- 当线程池被要求停止时，即使有任务可执行，线程也会停止等待并退出。

总之，这个等待条件实现了一个动态的等待机制，让线程在有任务可执行或者线程池停止时自动唤醒，以实现任务的分配和管理。

## enqueue

```c++
// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::invoke_result<F, Args...>::type> // return 返回值
{
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condtion.notify_one();
    return res;
}
```

这段代码定义了一个成员函数模板 `enqueue`，它接受一个可调用对象（函数、函数指针、成员函数等）以及其参数，并将任务添加到线程池中等待执行。

- `template<class F, class... Args>`
  这是一个模板声明，它告诉编译器这个函数是一个模板函数，可以接受不同类型的参数。

- `auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>`
  这是函数的声明，表明这个函数属于 `ThreadPool` 类，并且返回一个 `std::future` 对象。`ThreadPool::enqueue` 函数接受一个可调用对象 `f` 和一系列参数 `args...`。

- `using return_type = typename std::result_of<F(Args...)>::type;`
  这里使用 `std::result_of` 获取函数 `f` 的返回类型，并将其定义为 `return_type`。这是为了在稍后可以使用 `return_type` 来声明 `packaged_task`。

- `auto task = std::make_shared< std::packaged_task<return_type()> >(...)`
  创建一个 `packaged_task` 对象，将传入的可调用对象 `f` 和参数 `args...` 绑定在一起。`packaged_task` 是一个能够异步执行的任务。

- `std::future<return_type> res = task->get_future();`
  获取与 `packaged_task` 关联的 `future` 对象，以便在任务完成时可以获取任务的返回值。

- `std::unique_lock<std::mutex> lock(queue_mutex);`
  创建一个 `unique_lock` 对象并锁定互斥锁 `queue_mutex`，确保在添加任务到队列时是线程安全的。

- `if(stop) throw std::runtime_error("enqueue on stopped ThreadPool");`
  如果线程池已停止（`stop` 为 `true`），抛出异常，不允许继续添加任务。

- `tasks.emplace([task](){ (*task)(); });`
  将一个 lambda 函数添加到 `tasks` 队列中，lambda 函数内部会调用 `(*task)()`，即执行封装的任务。

- `condition.notify_one();`
  通知一个正在等待的线程，有新的任务可以执行。

- `return res;`
  返回 `future` 对象，以便在外部代码中可以通过该对象获取任务的返回值。

这个函数模板的作用是将任务添加到线程池中，以便线程池的工作线程可以异步执行这些任务，并在需要时获取任务的返回值。

## 调用函数方式1

```c++
// 函数声明
template<typename F, typename ... Args,
            typename return_type = typename std::invoke_result<F, Args...>::type>
    std::future<return_type> enqueue(F&& f, Args&&... args);
    
    
// 函数定义
template<typename F, typename ... Args,
        typename return_type = typename std::invoke_result<F, Args...>::type>
std::future<return_type> ThreadPool::enqueue(F&& f, Args&&... args)
{
    std::promise<return_type> promise;
    std::future<return_type> future = promise.get_future();
    /*
    auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    */
    auto task = [f = std::forward<F>(f),
            tup = std::make_tuple(std::forward<Args>(args)...),
            promise = std::move(promise)] () mutable {
        try {
            if constexpr (!std::is_void_v<return_type>) { // 不是void，进入这个分支获取返回值，
                return_type result = std::apply(std::move(f), std::move(tup));
                promise.set_value(result);
            } else { // 是void只执行这个，不获取返回值
                std::apply(std::move(f), std::move(tup));
            }
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    };

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task](){ (*task)(); });
    }
    condtion.notify_one();
    return future;
}
```

关于 "Template parameter redefines default argument" 错误，通常发生在多次定义相同模板参数的默认参数时。在您的情况下，这可能是因为您在函数声明和函数定义中都使用了 `return_type` 这个模板参数，默认参数只能在一处定义。

为了解决这个问题，您可以考虑将默认参数的定义放在函数模板的声明中，而不是在函数定义中。这样可以避免多处定义导致的问题。下面是示例代码：

```cpp
template<typename F, typename ... Args,
         typename return_type = typename std::invoke_result<F, Args...>::type>
std::future<return_type> enqueue(F&& f, Args&&... args);
```

在这个声明中，将 `return_type` 作为函数模板的默认参数。

然后，在函数定义中，不需要再次定义 `return_type`，直接使用即可：

```cpp
template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type>
ThreadPool::enqueue(F&& f, Args&&... args)
{
    // 函数定义的代码
}
```

通过将默认参数定义放在函数模板的声明中，您应该能够解决 "Template parameter redefines default argument" 错误。

或者直接修改

```c++
template<typename F, typename ... Args>
    std::future<typename std::invoke_result<F, Args...>::type> enqueue(F&& f, Args&&... args);
    
template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
{
```

## 函数调用方式2

```c++
template<typename F, typename ... Args>
std::future<typename std::invoke_result<F, Args...>::type> ThreadPool::enqueue(F&& f, Args&&... args)
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condtion.notify_one();
    return res;

}
```

这段代码是一个线程池中的任务添加函数的实现。它使用了可变参数模板 `ThreadPool::enqueue` 来接受一个函数 `f` 和其它的参数 `args...`。以下是每个部分的解释：

- `auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>`：这是函数签名，表示 `ThreadPool::enqueue` 函数会接受一个函数 `f` 和一系列参数 `args...`，并返回一个 `std::future`，其 `typename` 为 `std::result_of<F(Args...)>::type`，即函数 `f` 的返回类型。

- `using return_type = typename std::result_of<F(Args...)>::type;`：这里使用 `using` 关键字定义了一个类型别名 `return_type`，用来存储函数 `f` 的返回类型。`std::result_of` 是一个模板工具，用于获取函数的返回类型。

- `auto task = std::make_shared< std::packaged_task<return_type()> >(...)`：创建一个 `std::packaged_task` 对象，将传入的函数 `f` 绑定到任务中，并将任务存储在 `task` 智能指针中。

- `std::future<return_type> res = task->get_future();`：获取与任务关联的 `std::future`，以便在稍后获取任务的返回值。

- `std::unique_lock<std::mutex> lock(queue_mutex);`：获取互斥锁，确保多个线程不会同时修改任务队列。

- `if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");`：如果线程池已经停止，就抛出异常，不允许再添加任务。

- `tasks.emplace([task](){ (*task)(); });`：将任务添加到任务队列中，使用 lambda 表达式来执行任务。

- `condition.notify_one();`：唤醒一个等待中的线程，以便它可以从任务队列中获取任务执行。

- 返回 `res`，以便调用者可以获取任务的返回值。

这个函数的主要作用是将一个任务添加到线程池中，让线程池的线程来执行，并在需要时提供返回值。

```c++
auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
```

这行代码使用了 `std::make_shared` 来创建一个 `std::packaged_task` 对象，它会封装要执行的任务。`std::bind` 函数将传递给 `enqueue` 函数的函数对象 `f` 以及其余的参数 `args` 绑定到这个 `std::packaged_task` 中，从而构造了一个可调用对象，当执行该对象时，实际上就是在执行 `f(args...)`。这个 `std::packaged_task` 对象可以被调用，也可以用于获取异步操作的结果。

在这个整体结构中，通过将任务函数和参数绑定到 `std::packaged_task` 对象中，实现了将任务投递到线程池中异步执行，并且通过 `std::future` 对象来获取任务执行的结果。这是一种方便的方式来管理异步任务的执行和结果获取。

```c++
tasks.emplace([task](){ (*task)(); });
```

这行代码的作用是将一个任务添加到任务队列 `tasks` 中，使用 lambda 表达式来执行任务。

解释一下这行代码的每个部分：

- `tasks.emplace(...)`: 这是将一个任务添加到 `tasks` 队列中的操作，使用 `emplace` 方法可以在队列尾部构造一个新元素。

- `[task](){ (*task)(); }`: 这是一个 lambda 表达式，用来执行任务。Lambda 表达式内部的代码会在被调用时执行。在这里，lambda 表达式接收捕获了一个名为 `task` 的外部变量，并在 lambda 表达式内部使用它。

- `(*task)();`: 这是在 lambda 表达式内部执行的代码。它调用了 `task` 智能指针所指向的任务，实际上就是执行任务函数。通过 `(*task)()` 的方式来调用这个任务。

综合起来，这行代码的作用是将一个任务（即 lambda 表达式）添加到任务队列中，当任务在队列中被取出并执行时，它会调用 `task` 指向的函数，即执行该任务的具体操作。