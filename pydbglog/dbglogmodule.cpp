#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/call.hpp>

#include <stdint.h>

#include <dbglog/dbglog.hpp>

namespace python = boost::python;

namespace dbglog { namespace py {

python::object extractStack;
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

python::object log(dbglog::level level, const std::string &prefix
                   , python::tuple args, python::dict kwargs)
{
    using namespace python;

    if (!dbglog::detail::deflog.check_level(level)) {
        return object(false);
    }

    object frame(extractStack(object(), 1)[0]);

    const char *file = extract<const char *>(frame[0])();
    const char *func = extract<const char *>(frame[2])();
    size_t lineno = extract<size_t>(frame[1])();

    // ensure string is in unicode
    object fs(args[0]);
    if (!PyUnicode_Check(fs.ptr())) {
        fs = fs.attr("decode")("utf-8");
        if (!fs) { return fs; }
    }

    str msg(fs.attr("format")(*(args[slice(1,_)]), **kwargs)
            .attr("encode")("utf-8"));
    if (!msg) { return msg; }

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
    FUNCTION_PAIR(log_console);

#undef FUNCTION_PAIR

    def("log_file", &py::log_file);
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

    py::extractStack = import("traceback").attr("extract_stack");
}


namespace dbglog { namespace py {

boost::python::object import()
{
    static bool imported(false);
    if (!imported) {
        initdbglog();
        imported = true;
    }
    return boost::python::import("dbglog");
}

} } // namespace dbglog::py
