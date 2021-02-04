/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <string>
#include <vector>
#include <mutex>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/call.hpp>

#include <stdint.h>

#include "dbglog/dbglog.hpp"

namespace python = boost::python;

namespace dbglog { namespace py {

PyObject *extractStack;

const std::string empty = std::string();

python::object logHook = python::object();

void log_hook()
{
    logHook = python::object();
}

void log_hook(python::object hook)
{
    logHook = hook;
}

namespace format {

using namespace boost::python;

class FormatProxy {
public:
    FormatProxy(object arg)
        : format_(arg.attr("__format__"))
    {}

    object format(object spec) {
        object res(format_(spec));

#if PY_MAJOR_VERSION == 2
        if (PyUnicode_Check(res.ptr())) {
            return res.attr("encode")("utf-8");
        }
#endif
        return res;
    }

private:
    object format_;
};

object FormatProxyClass;

/** Wraps positional arguments but ignores the first one (this works like
 *  working on args_[1:] slice.
 */
class ArgsWrapper {
public:
    ArgsWrapper(object args)
        : args_(args)
    {}

    long len() {
        auto l(python::len(args_));
        if (l) { return l - 1; }
        return l;
    }

    object getitem(object i) {
        return FormatProxyClass(object(args_[i + 1]));
    }

private:
    object args_;
};

object ArgsWrapperClass;

class KwArgsWrapper {
public:
    KwArgsWrapper(object args)
        : args_(args)
    {}

    object keys() {
        return args_.attr("keys")();
    }

    long len() {
        return python::len(args_);
    }

    object getitem(object key) {
        return FormatProxyClass(object(args_[key]));
    }

private:
    object args_;
};

object KwArgsWrapperClass;

} // namespace format

python::object log(dbglog::level level, const std::string &prefix
                   , python::tuple args, python::dict kwargs)
{
    using namespace python;

    if (!dbglog::detail::deflog.check_level(level)) {
        return object(false);
    }

    object frame(python::call<python::object>(extractStack, object(), 1)[0]);

    const char *file = extract<const char *>(frame[0])();
    const char *func = extract<const char *>(frame[2])();
    size_t lineno = extract<size_t>(frame[1])();

    // get format string
    object fstr(args[0]);
#if PY_MAJOR_VERSION == 2
    if (PyUnicode_Check(fstr.ptr())) {
        fstr = fstr.attr("encode")("utf-8");
    }
#endif

    str msg(fstr.attr("format")(*format::ArgsWrapperClass(args)
                                , **format::KwArgsWrapperClass(kwargs)));

    python::object res(dbglog::detail::deflog.prefix_log
                       (level, prefix, extract<const char *>(msg)()
                        , dbglog::location(file, func, lineno, true)));

    if (logHook) {
        logHook(python::object(dbglog::detail::level2string(level))
                , python::object(prefix), msg);
    }

    return res;
}

#define LOG_FUNCTION(PREFIX, LEVEL)                                     \
    python::object LEVEL(python::tuple args, python::dict kwargs) {     \
        return log(dbglog::LEVEL, PREFIX, args, kwargs);                \
    }

    LOG_FUNCTION(empty, debug)

    LOG_FUNCTION(empty, info1)
    LOG_FUNCTION(empty, info2)
    LOG_FUNCTION(empty, info3)
    LOG_FUNCTION(empty, info4)

    LOG_FUNCTION(empty, warn1)
    LOG_FUNCTION(empty, warn2)
    LOG_FUNCTION(empty, warn3)
    LOG_FUNCTION(empty, warn4)

    LOG_FUNCTION(empty, err1)
    LOG_FUNCTION(empty, err2)
    LOG_FUNCTION(empty, err3)
    LOG_FUNCTION(empty, err4)

    LOG_FUNCTION(empty, fatal)

class module {
public:
    module(const std::string &name)
        : name_(name), prefix_("[" + name + "]")
    {}

    module(const std::string &name, module other)
        : name_(other.name_ + "/" + name)
        , prefix_("[" + other.name_ + "/" + name + "]")
    {}

    LOG_FUNCTION(prefix_, debug)

    LOG_FUNCTION(prefix_, info1)
    LOG_FUNCTION(prefix_, info2)
    LOG_FUNCTION(prefix_, info3)
    LOG_FUNCTION(prefix_, info4)

    LOG_FUNCTION(prefix_, warn1)
    LOG_FUNCTION(prefix_, warn2)
    LOG_FUNCTION(prefix_, warn3)
    LOG_FUNCTION(prefix_, warn4)

    LOG_FUNCTION(prefix_, err1)
    LOG_FUNCTION(prefix_, err2)
    LOG_FUNCTION(prefix_, err3)
    LOG_FUNCTION(prefix_, err4)

    LOG_FUNCTION(prefix_, fatal)

    python::object throw_(python::object excType);

    const std::string& prefix() const {
        return prefix_;
    }

private:
    std::string name_;
    std::string prefix_;
};

#undef LOG_FUNCTION

#define LOG_FUNCTION(LEVEL)                                             \
    python::object module_##LEVEL##_raw(python::tuple args              \
                                        , python::dict kwargs) {        \
        return python::object                                           \
            (python::extract<module&>(args[0])()                        \
             .LEVEL(python::tuple(args.slice(1, python::_)), kwargs));  \
    }

    LOG_FUNCTION(debug)
    LOG_FUNCTION(info1)
    LOG_FUNCTION(info2)
    LOG_FUNCTION(info3)
    LOG_FUNCTION(info4)
    LOG_FUNCTION(warn1)
    LOG_FUNCTION(warn2)
    LOG_FUNCTION(warn3)
    LOG_FUNCTION(warn4)
    LOG_FUNCTION(err1)
    LOG_FUNCTION(err2)
    LOG_FUNCTION(err3)
    LOG_FUNCTION(err4)
    LOG_FUNCTION(fatal)

#undef LOG_FUNCTION

class thrower {
public:
    thrower(python::object excType)
        : excType_(excType)
    {}

    void raise(const char *msg) {
        PyErr_SetString(excType_.ptr(), msg);
        python::throw_error_already_set();
    }

private:
    python::object excType_;
};

// FIXME: use unicode as well
#define LOG_FUNCTION(LEVEL)                                             \
    python::object throw_##LEVEL(python::tuple args                     \
                                 , python::dict kwargs)                 \
    {                                                                   \
        thrower &t = python::extract<thrower&>(args[0])();                \
        python::tuple logArgs = python::tuple(args.slice(1, python::_)); \
        python::tuple msgArgs = python::tuple(args.slice(2, python::_)); \
        log(dbglog::LEVEL, empty, logArgs, kwargs);                     \
        python::str msg(args[1].attr("format")(*msgArgs, **kwargs));    \
        t.raise(python::extract<const char*>(msg)());                   \
        return python::object();                                        \
    }

    LOG_FUNCTION(debug)
    LOG_FUNCTION(info1)
    LOG_FUNCTION(info2)
    LOG_FUNCTION(info3)
    LOG_FUNCTION(info4)
    LOG_FUNCTION(warn1)
    LOG_FUNCTION(warn2)
    LOG_FUNCTION(warn3)
    LOG_FUNCTION(warn4)
    LOG_FUNCTION(err1)
    LOG_FUNCTION(err2)
    LOG_FUNCTION(err3)
    LOG_FUNCTION(err4)
    LOG_FUNCTION(fatal)

#undef LOG_FUNCTION

python::object throw_(python::object excType)
{
    return python::object(thrower(excType));
}

class module_thrower {
public:
    module_thrower(const module &module, python::object excType)
        : module_(module), excType_(excType)
    {}

    void raise(const char *msg) {
        PyErr_SetString(excType_.ptr(), msg);
        python::throw_error_already_set();
    }

    const module& get_module() const {
        return module_;
    }

private:
    module module_;
    python::object excType_;
};

// FIXME: use unicode as well
#define LOG_FUNCTION(LEVEL)                                             \
    python::object module_throw_##LEVEL(python::tuple args              \
                                        , python::dict kwargs)          \
    {                                                                   \
        module_thrower &t = python::extract<module_thrower&>(args[0])(); \
        python::tuple logArgs = python::tuple(args.slice(1, python::_)); \
        python::tuple msgArgs = python::tuple(args.slice(2, python::_)); \
        log(dbglog::LEVEL, t.get_module().prefix(), logArgs, kwargs);   \
        python::str msg(args[1].attr("format")(*msgArgs, **kwargs));    \
        t.raise(python::extract<const char*>(msg)());                   \
        return python::object();                                        \
    }

    LOG_FUNCTION(debug)
    LOG_FUNCTION(info1)
    LOG_FUNCTION(info2)
    LOG_FUNCTION(info3)
    LOG_FUNCTION(info4)
    LOG_FUNCTION(warn1)
    LOG_FUNCTION(warn2)
    LOG_FUNCTION(warn3)
    LOG_FUNCTION(warn4)
    LOG_FUNCTION(err1)
    LOG_FUNCTION(err2)
    LOG_FUNCTION(err3)
    LOG_FUNCTION(err4)
    LOG_FUNCTION(fatal)

#undef LOG_FUNCTION

python::object module::throw_(python::object excType)
{
    return python::object(module_thrower(*this, excType));
}

void set_mask(python::str &m)
{
    using namespace python;

    const char *mask = extract<const char *>(m)();
    dbglog::set_mask(dbglog::mask(mask));
}

python::str get_mask()
{
    return dbglog::get_mask_string().c_str();
}

void log_thread()
{
    return dbglog::log_thread();
}

void log_thread(bool value)
{
    return dbglog::log_thread(value);
}

void log_pid()
{
    return dbglog::log_pid();
}

void log_pid(bool value)
{
    return dbglog::log_pid(value);
}

void log_console()
{
    return dbglog::log_console();
}

void log_console(bool value)
{
    return dbglog::log_console(value);
}

bool get_log_console()
{
    return dbglog::get_log_console();
}

bool log_file(const std::string &filename)
{
    return dbglog::log_file(filename);
}

python::str thread_id()
{
    return python::str(dbglog::thread_id());
}

void thread_id(python::str value)
{
    dbglog::thread_id(python::extract<std::string>
                      (value.attr("encode")("utf-8"))());
}

} } // namespace dbglog::py

/** Python bindings.
 */
BOOST_PYTHON_MODULE(dbglog)
{
    using namespace python;
    namespace py = dbglog::py;

#define LOG_FUNCTION(LEVEL) \
    def(#LEVEL, raw_function(py::LEVEL, 1))

    LOG_FUNCTION(debug);
    LOG_FUNCTION(info1);
    LOG_FUNCTION(info2);
    LOG_FUNCTION(info3);
    LOG_FUNCTION(info4);
    LOG_FUNCTION(warn1);
    LOG_FUNCTION(warn2);
    LOG_FUNCTION(warn3);
    LOG_FUNCTION(warn4);
    LOG_FUNCTION(err1);
    LOG_FUNCTION(err2);
    LOG_FUNCTION(err3);
    LOG_FUNCTION(err4);
    LOG_FUNCTION(fatal);

#undef LOG_FUNCTION

#define LOG_FUNCTION(LEVEL) \
    def(#LEVEL, raw_function(&py::module_##LEVEL##_raw, 2))

    class_<py::module>("module", init<std::string>())
        .def(init<std::string, py::module>())
        .LOG_FUNCTION(debug)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(info1)
        .LOG_FUNCTION(info2)
        .LOG_FUNCTION(info3)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(warn1)
        .LOG_FUNCTION(warn2)
        .LOG_FUNCTION(warn3)
        .LOG_FUNCTION(warn4)
        .LOG_FUNCTION(err1)
        .LOG_FUNCTION(err2)
        .LOG_FUNCTION(err3)
        .LOG_FUNCTION(err4)
        .LOG_FUNCTION(fatal)
        .def("throw", &py::module::throw_)
        ;

#undef LOG_FUNCTION

    def("set_mask", py::set_mask);
    def("get_mask", py::get_mask);

    typedef void (*void_function)();
    typedef void (*void_bool_function)(bool);

#define FUNCTION_PAIR(NAME) \
    def(#NAME, static_cast<void_function>(&py::NAME)); \
    def(#NAME, static_cast<void_bool_function>(&py::NAME))

    FUNCTION_PAIR(log_thread);
    FUNCTION_PAIR(log_pid);
    def<void()>("log_console", py::log_console);
    def<void(bool)>("log_console", py::log_console);
    def("get_log_console", py::get_log_console);

#undef FUNCTION_PAIR

    def("log_file", &py::log_file);

    typedef void (*void_object_function)(python::object);
    def("log_hook", static_cast<void_function>(&py::log_hook));
    def("log_hook", static_cast<void_object_function>(&py::log_hook));

    typedef str (*string_function)();
    typedef void (*void_string_function)(python::str);
    def("thread_id", static_cast<string_function>(&py::thread_id));
    def("thread_id", static_cast<void_string_function>(&py::thread_id));

#define LOG_FUNCTION(LEVEL) \
    def(#LEVEL, raw_function(&py::throw_##LEVEL, 2))

    class_<py::thrower>("__thrower", init<object>())
        .def("throw", &py::thrower::raise)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(info1)
        .LOG_FUNCTION(info2)
        .LOG_FUNCTION(info3)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(warn1)
        .LOG_FUNCTION(warn2)
        .LOG_FUNCTION(warn3)
        .LOG_FUNCTION(warn4)
        .LOG_FUNCTION(err1)
        .LOG_FUNCTION(err2)
        .LOG_FUNCTION(err3)
        .LOG_FUNCTION(err4)
        .LOG_FUNCTION(fatal)
        ;
#undef LOG_FUNCTION

    def("throw", &py::throw_);

#define LOG_FUNCTION(LEVEL) \
    def(#LEVEL, raw_function(&py::module_throw_##LEVEL, 2))

    class_<py::module_thrower>
        ("__module_thrower", init<const py::module&, object>())
        .def("throw", &py::module_thrower::raise)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(info1)
        .LOG_FUNCTION(info2)
        .LOG_FUNCTION(info3)
        .LOG_FUNCTION(info4)
        .LOG_FUNCTION(warn1)
        .LOG_FUNCTION(warn2)
        .LOG_FUNCTION(warn3)
        .LOG_FUNCTION(warn4)
        .LOG_FUNCTION(err1)
        .LOG_FUNCTION(err2)
        .LOG_FUNCTION(err3)
        .LOG_FUNCTION(err4)
        .LOG_FUNCTION(fatal)
        ;
#undef LOG_FUNCTION

    py::format::ArgsWrapperClass = class_<py::format::ArgsWrapper>
        ("ArgsWrapper", init<object>())
        .def("__len__", &py::format::ArgsWrapper::len)
        .def("__getitem__", &py::format::ArgsWrapper::getitem)
        ;

    py::format::KwArgsWrapperClass = class_<py::format::KwArgsWrapper>
        ("KwArgsWrapper", init<object>())
        .def("keys", &py::format::KwArgsWrapper::keys)
        .def("__len__", &py::format::KwArgsWrapper::len)
        .def("__getitem__", &py::format::KwArgsWrapper::getitem)
        ;

    py::format::FormatProxyClass = class_<py::format::FormatProxy>
        ("ArgProxy", init<object>())
        .def("__format__", &py::format::FormatProxy::format)
        ;

    // get traceback.extract_stack;
    object es(import("traceback").attr("extract_stack"));

    // hold it in this module
    python::scope().attr("_extract_stack") = es;

    // and get pointer to it so functionas can use it
    py::extractStack = es.ptr();
}

namespace dbglog { namespace py {

namespace {
std::once_flag onceFlag;
} // namespace

boost::python::object import()
{
    std::call_once(onceFlag, [&]()
    {
#if PY_MAJOR_VERSION == 2
        initdbglog();
#else
        typedef python::handle< ::PyObject> Handle;
        Handle module(PyInit_dbglog());
        auto sys(python::import("sys"));
        sys.attr("modules")["dbglog"] = module;
#endif
    });

    return boost::python::import("dbglog");
}

} } // namespace dbglog::py
