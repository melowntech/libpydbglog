#ifndef shared_dbglog_python_dbglogmodule_hpp_included_
#define shared_dbglog_python_dbglogmodule_hpp_included_

#include <boost/python/object.hpp>

namespace dbglog { namespace py {

/** Imports built-in dbglog module.
 */
boost::python::object import();

} } // namespace dbglog::py

#endif // shared_dbglog_python_dbglogmodule_hpp_included_
