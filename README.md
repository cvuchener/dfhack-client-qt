DFHack remote client library for Qt applications
================================================

How to build
------------

This library depends on Protocol Buffers and Qt5 with Network module. Qt5's
Widgets module is also required for the console example.

### CMake options

 - `BUILD_CONSOLE_EXAMPLE`: build the remote console example (default: `OFF`).


How to use
----------

You need to create [Client](dfhack-client-qt/Client.h) object and call the
`connect` method. A `connectionChanged` signal will be emitted when the client
is ready for remote function calls. If the connection fails a `socketError`
signal is emitted instead.

The best way to call remote function is to use
[Function](dfhack-client-qt/Function.h) objects. You can check
[Core.h](dfhack-client-qt/Core.h) for an example on how to conveniently declare
Function types. Functions must be bound before the can be called. Bind
operations and calls are asynchronous, they return immediately and completion
is either signaled through the `bind_notifier` and `call_notifier` members (see
[notifier classes](dfhack-client-qt/Notifier.h)) or with the `std::future`
returned from the methods.

### Synchronous example

Use `std::future::get` for waiting until completion.

```c++
MyFunction my_function(&client);

// bind function
if (!my_function.bind().get()) {
    // handle error here
}

// set parameters
my_function.in.set_foo("bar");

// call function
if (my_function.call().get() != DFHack::CommandResult::Ok) {
    // handle error here
}

// get results
auto result = my_function.out.foo();

// ...
```

### Asynchronous signal example

```c++
// in constructor

    // connect signals
    connect(&client, &DFHack::Client::connectionChanged,
            this, &Example::onConnectionChanged);
    connect(&my_function.bind_notifier, &DFHack::BindNotifier::bound,
            this, &Example::onFunctionBound);
    connect(&my_function.call_notifier, &DFHack::CallNotifier::finished,
            this, &Example::onFunctionFinished);

// ...

void Example::onConnectionChanged(bool connected) {
    if (connected) {
        // functions can now be bound
        my_function.bind()
    }
}

void Example::onFunctionBound(bool success) {
    if (!success) {
        // handle error here
        return;
    }

    // the function can now be called
    my_function.in.set_foo("bar");
    my_function.call();
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
