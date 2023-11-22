DFHack remote client library for Qt applications
================================================

How to build
------------

This library depends on Protocol Buffers and Qt6 with Network module. Qt6's
Widgets module is also required for the console example.

### CMake options

 - `BUILD_CONSOLE_EXAMPLE`: build the remote console example (default: `OFF`).
 - `BUILD_TEST`: build test examples in the `test` directory (default: `OFF`).


How to use
----------

You need to create [Client](dfhack-client-qt/Client.h) object and call the
`connect` method. A `connectionChanged` signal will be emitted when the client
is ready for remote function calls. If the connection fails a `socketError`
signal is emitted instead. You can also use the returned QFuture to check the
connection success.

The best way to call remote function is to use
[Function](dfhack-client-qt/Function.h) objects. You can check
[Core.h](dfhack-client-qt/Core.h) or [Basic.h](dfhack-client-qt/Basic.h) for an
example on how to conveniently declare Function objects.

Functions may be bound before they can be called. If not already bound the
function will be bound on the first call. Bind operations and calls are
asynchronous, they return immediately a QFuture (or pair of QFuture).

### Synchronous example

Use `QFuture::waitForFinished` to block until the call is finished. The client
must be run in another thread. Calls will not progress if the client's thread
is blocked.

```c++
const MyFunction my_function{"MyPlugin", "MyFunction"};

// optional: bind function
auto br = my_function.bind(client);
if (!br.result()) {
    // handle bind error here
}

// set parameters
auto my_function_args = my_function.args();
my_function_args.set_foo("bar");

// call function
auto [reply, notifications] = my_function(client, my_function_args);
auto res = reply.result();
if (!res) {
    // handle error here
    std::cerr << "Failed to call my_function: "
              << make_error_code(res.cr).message()
              << std::endl;
}
else {
    // get result content
    auto foo = res->foo();
}
```

See also example in [test-sync](test/test-sync.cpp).


### Asynchronous signal with QFutureWatcher example

Use QFutureWatcher to get signals from futures.

```c++
// in constructor

    // connect signals
    connect(&client, &DFHack::Client::connectionChanged,
            this, &Example::onConnectionChanged);
    connect(&command_watcher, &QFutureWatcher<DFHack::CallReply<MyFunctionReply>>::finished,
            this, &Example::onFunctionFinished);

// ...

void Example::onConnectionChanged(bool connected) {
    if (connected) {
        // the function can now be called
        auto my_function_args = my_function.args();
        my_function_args.set_foo("bar");
        command_watcher.setFuture(my_function(client, my_function_args).first);
    }
}

void Example::onFunctionFinished(DFHack::CallReply<MyFunctionReply> reply)
{
    if (!reply) {
        // handle error here
    }
    else {
        auto result = reply->foo();
        // ...
    }
}
```

See also the remote console example in the `console` directory.


Licenses
--------

dfhack-client-qt is library is distributed under LGPLv3.

console example is distributed under GPLv3.

Protocol buffers definitions (.proto files in dfhack-client-qt) are from the
[DFHack](https://github.com/DFHack/dfhack/) project and distributed under Zlib
license.
