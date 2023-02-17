DFHack remote client library for Qt applications
================================================

How to build
------------

This library depends on Protocol Buffers and Qt6 with Network module. Qt6's
Widgets module is also required for the console example.

### CMake options

 - `BUILD_CONSOLE_EXAMPLE`: build the remote console example (default: `OFF`).


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
example on how to conveniently declare Function types. Functions must be bound
before the can be called. Bind operations and calls are asynchronous, they
return immediately. 

### Synchronous example

Use `QFuture::waitForFinished` to block until the call is finished. The client
must be run in another thread. Calls will not progress if the client's thread
is blocked.

```c++
MyFunction my_function(&client);

// bind function
auto br = my_function.bind();
br.waitForFinished();
if (!br.result()) {
    // handle error here
}

// set parameters
my_function.in.set_foo("bar");

// call function
auto [cr, notifications] = my_function.call();
cr.waitForFinished();
if (cr.result() != DFHack::CommandResult::Ok) {
    // handle error here
}

// get results
auto result = my_function.out.foo();

// ...
```

See also example in [test-sync](test/test-sync.cpp).


### Asynchronous signal with QFutureWatcher example

Use QFutureWatcher to get signals from futures.

```c++
// in constructor

    // connect signals
    connect(&client, &DFHack::Client::connectionChanged,
            this, &Example::onConnectionChanged);
    connect(&bind_watcher, &QFutureWatcher<bool>::finished,
            this, &Example::onFunctionBound);
    connect(&command_watcher, &QFutureWatcher<DFHack::CommandResult>::finished,
            this, &Example::onFunctionFinished);

// ...

void Example::onConnectionChanged(bool connected) {
    if (connected) {
        // functions can now be bound
        bind_watcher.setFuture(my_function.bind());
    }
}

void Example::onFunctionBound(bool success) {
    if (!success) {
        // handle error here
        return;
    }

    // the function can now be called
    my_function.in.set_foo("bar");
    command_watcher.setFuture(my_function.call().first);
}

void Example::onFunctionFinished(DFHack::CommandResult results)
{
    if (results != DFHack::CommandResult::Ok) {
        // handle error here
    }
    else {
        auto result = my_function.out.foo();
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
