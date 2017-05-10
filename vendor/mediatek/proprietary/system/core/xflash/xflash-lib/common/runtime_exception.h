#ifndef __RUNTIME_EXCEPTION__
#define __RUNTIME_EXCEPTION__

#include <boost/exception/all.hpp>
#include <exception>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <boost/format.hpp>

typedef boost::error_info<struct tag_errmsg, std::string> errmsg_info;
typedef boost::error_info<struct tag_errappendix, int> err_appendix;

//std::cerr << *boost::get_error_info<errmsg_info>(e);

//usage: BOOST_THROW_EXCEPTION(runtime_exception(string("Unknow error")));

class runtime_exception : virtual public std::exception, virtual public boost::exception
{
public:
  runtime_exception(std::string msg)
  : what_(msg)
  {
  }
  runtime_exception(boost::format fmt_msg)
  : what_(fmt_msg.str())
  {
  }
  ~runtime_exception() throw(){}

  virtual const char *what() const throw()
  {
    return what_.c_str();
  }

private:
  std::string what_;
};

class exception_abort : public runtime_exception
{
public:
  exception_abort()
  : runtime_exception("operation abort.")
  {
  }
  ~exception_abort() throw(){}

};

class exception_timeout : public runtime_exception
{
public:
  exception_timeout()
  : runtime_exception("operation timeout.")
  {
  }
  ~exception_timeout() throw(){}

};

#endif
