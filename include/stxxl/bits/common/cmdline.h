/***************************************************************************
 *  include/stxxl/bits/common/cmdline.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_CMDLINE_HEADER
#define STXXL_COMMON_CMDLINE_HEADER

#include <string>
#include <iostream>
#include <vector>
#include <iomanip>

#include <string.h>
#include <stdio.h>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/utils.h>

__STXXL_BEGIN_NAMESPACE

/**
 * \brief Command line parser which automatically fills variables and prints
 * nice usage messages.
 *
 * This is a straightforward command line parser in C++, which will recognize
 * short options -s, long options --long and parameters, both required and
 * optional. It will automatically parse integers and <b>byte sizes</b> with
 * SI/IEC suffixes (e.g. 1 GiB). It also works with lists of strings,
 * e.g. multiple filenames.
 *
 * Maybe most important it will nicely format the options and parameters
 * description using word wrapping.
 */
class cmdline_parser : private noncopyable
{
protected:

    //! base class of all options and parameters
    struct argument
    {
        //! single letter short option, or 0 is none
        char        m_key;
        //! long option key or name for parameters
        std::string m_longkey;
        //! option type description, e.g. "<#>" to indicate numbers
        std::string m_keytype;
        //! longer description, which will be wrapped
        std::string m_desc;
        //! required, process() fails if the option/parameter is not found.
        bool        m_required;
        //! found during processing of command line
        bool        m_found;
        //! repeated argument, i.e. std::vector<std::string>
        bool        m_repeated;

        //! contructor filling most attributes
        argument(char key, const std::string& longkey, const std::string& keytype,
                 const std::string& desc, bool required)
            : m_key(key), m_longkey(longkey), m_keytype(keytype),
              m_desc(desc), m_required(required), m_found(false), m_repeated(false)
        {
        }

        //! empty virtual destructor
        virtual ~argument() { }

        //! return formatted type name to user
        virtual const char* type_name() const = 0;

        //! process one item from command line for this argument
        virtual bool process(int& argc, const char* const* &argv) = 0;

        //! format value to ostream
        virtual void print_value(std::ostream& os) const = 0;

        //! return 'longkey [keytype]'
        std::string param_text() const
        {
            std::string s = m_longkey;
            if (m_keytype.size()) {
                s += ' '; s += m_keytype;
            }
            return s;
        }

        //! return '-s, --longkey [keytype]'
        std::string option_text() const
        {
            std::string s;
            if (m_key) { 
                s += '-'; s += m_key; s += ", ";
            }
            s += "--";
            s += m_longkey;
            if (m_keytype.size()) {
                s += ' '; s += m_keytype;
            }
            return s;
        }
    };

    //! specialization of argument for boolean flags (can only be set to true).
    struct argument_flag : public argument
    {
        //! reference to boolean to set to true
        bool&       m_dest;

        //! contructor filling most attributes
        argument_flag(char key, const std::string& longkey, const std::string& keytype,
                      const std::string& desc, bool required, bool& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
        }

        virtual const char* type_name() const
        { return "flag"; }

        //! "process" argument: just set to true, no argument is used.
        virtual bool process(int&, const char* const* &)
        {
            m_dest = true;
            return true;
        }

        virtual void print_value(std::ostream& os) const
        { os << (m_dest ? "true" : "false"); }
    };

    //! specialization of argument for integer options or parameters
    struct argument_int : public argument
    {
        int& m_dest;

        //! contructor filling most attributes
        argument_int(char key, const std::string& longkey, const std::string& keytype,
                     const std::string& desc, bool required, int& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
        }

        virtual const char* type_name() const
        { return "integer"; }

        //! parse signed integer using sscanf.
        virtual bool process(int& argc, const char* const* &argv)
        {
            if (argc == 0) return false;
            if (sscanf(argv[0], "%d", &m_dest) == 1) {
                --argc, ++argv;
                return true;
            } else {
                return false;
            }
        }

        virtual void print_value(std::ostream& os) const
        { os << m_dest; }
    };

    //! specialization of argument for unsigned integer options or parameters
    struct argument_uint : public argument
    {
        unsigned int& m_dest;

        //! contructor filling most attributes
        argument_uint(char key, const std::string& longkey, const std::string& keytype,
                     const std::string& desc, bool required, unsigned int& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
        }

        virtual const char* type_name() const
        { return "unsigned integer"; }

        //! parse unsigned integer using sscanf.
        virtual bool process(int& argc, const char* const* &argv)
        {
            if (argc == 0) return false;
            if (sscanf(argv[0], "%u", &m_dest) == 1) {
                --argc, ++argv;
                return true;
            } else {
                return false;
            }
        }

        virtual void print_value(std::ostream& os) const
        { os << m_dest; }
    };

    //! specialization of argument for SI/IEC suffixes byte size options or parameters
    struct argument_bytes : public argument
    {
        uint64& m_dest;

        //! contructor filling most attributes
        argument_bytes(char key, const std::string& longkey, const std::string& keytype,
                     const std::string& desc, bool required, uint64& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
        }

        virtual const char* type_name() const
        { return "bytes"; }

        //! parse byte size using SI/IEC parser from stxxl.
        virtual bool process(int& argc, const char* const* &argv)
        {
            if (argc == 0) return false;
            if (parse_SI_IEC_size(argv[0], m_dest)) {
                --argc, ++argv;
                return true;
            } else {
                return false;
            }
        }

        virtual void print_value(std::ostream& os) const
        { os << m_dest; }
    };

    //! specialization of argument for string options or parameters
    struct argument_string : public argument
    {
        std::string& m_dest;

        //! contructor filling most attributes
        argument_string(char key, const std::string& longkey, const std::string& keytype,
                        const std::string& desc, bool required, std::string& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
        }

        virtual const char* type_name() const
        { return "string"; }

        //! "process" string argument just by storing it.
        virtual bool process(int& argc, const char* const* &argv)
        {
            if (argc == 0) return false;
            m_dest = argv[0];
            --argc, ++argv;
            return true;
        }

        virtual void print_value(std::ostream& os) const
        { os << '"' << m_dest << '"'; }
    };

    //! specialization of argument for multiple string options or parameters
    struct argument_stringlist : public argument
    {
        std::vector<std::string>& m_dest;

        //! contructor filling most attributes
        argument_stringlist(char key, const std::string& longkey, const std::string& keytype,
                            const std::string& desc, bool required, std::vector<std::string>& dest)
            : argument(key, longkey, keytype, desc, required),
              m_dest(dest)
        {
            m_repeated = true;
        }

        virtual const char* type_name() const
        { return "string list"; }

        //! "process" string argument just by storing it in vector.
        virtual bool process(int& argc, const char* const* &argv)
        {
            if (argc == 0) return false;
            m_dest.push_back(argv[0]);
            --argc, ++argv;
            return true;
        }

        virtual void print_value(std::ostream& os) const
        {
            os << '[';
            for (size_t i = 0; i < m_dest.size(); ++i)
            {
                if (i != 0) os << ',';
                os << '"' << m_dest[i] << '"';
            }
            os << ']';
        }
    };

protected:

    //! option and parameter list type
    typedef std::vector<argument*> arglist_type;

    //! list of options available
    arglist_type        m_optlist;
    //! list of parameters, both required and optional
    arglist_type        m_paramlist;

    //! formatting width for options, '-s, --switch <#>'
    size_t              m_opt_maxlong;
    //! formatting width for parameters, 'param <#>'
    size_t              m_param_maxlong;

    //! argv[0] for usage.
    const char*         m_progname;

    //! verbose processing of arguments
    bool                m_verbose_process;

    //! user set description of program, will be wrapped
    std::string         m_description;
    //! user set author of program, will be wrapped
    std::string         m_author;

    //! set line wrap length
    unsigned int        m_linewrap;

    //! maximum length of a type_name() result
    static const int m_maxtypename = 16;

private:

    //! update maximum formatting width for new option
    void calc_opt_max(const argument* arg)
    {
        m_opt_maxlong = std::max(arg->option_text().size()+2, m_opt_maxlong);
    }

    //! update maximum formatting width for new parameter
    void calc_param_max(const argument* arg)
    {
        m_param_maxlong = std::max(arg->param_text().size()+2, m_param_maxlong);
    }

public:

    //! Wrap a long string at spaces into lines. Prefix is added
    //! unconditionally to each line. Lines are wrapped after wraplen
    //! characters if possible.
    static inline void
    output_wrap(std::ostream& os, const std::string& text, size_t wraplen,
                size_t indent_first = 0, size_t indent_rest = 0,
                size_t current = 0, size_t indent_newline = 0)
    {
        std::string::size_type t = 0;
        size_t indent = indent_first;
        while (t != text.size())
        {
            std::string::size_type to = t, lspace = t;

            // scan forward in text until we hit a newline or wrap point
            while (to != text.size() &&
                   to + current + indent < t + wraplen &&
                   text[to] != '\n')
            {
                if (text[to] == ' ') lspace = to;
                ++to;
            }
            
            // go back to last space
            if (to != text.size() && text[to] != '\n' &&
                lspace != t) to = lspace+1;

            // output line
            os << std::string(indent, ' ')
               << text.substr(t, to - t) << std::endl;

            current = 0;
            indent = indent_rest;

            // skip over last newline
            if (to != text.size() && text[to] == '\n') {
                indent = indent_newline;
                ++to;
            }

            t = to;
        }
    }

public:

    //! Construct new command line parser
    cmdline_parser()
        : m_opt_maxlong(8),
          m_param_maxlong(8),
          m_progname(NULL),
          m_verbose_process(true),
          m_linewrap(80)
    {
    }

    //! Delete all added arguments
    ~cmdline_parser()
    {
        for (size_t i = 0; i < m_optlist.size(); ++i)
            delete m_optlist[i];
        m_optlist.clear();

        for (size_t i = 0; i < m_paramlist.size(); ++i)
            delete m_paramlist[i];
        m_paramlist.clear();
    }

    //! Set description of program, text will be wrapped
    void set_description(const std::string& description)
    {
        m_description = description;
    }

    //! Set author of program, will be wrapped.
    void set_author(const std::string& author)
    {
        m_author = author;
    }

    //! Set verbose processing of command line arguments
    void set_verbose_process(bool verbose_process)
    {
        m_verbose_process = verbose_process;
    }

    // ************************************************************************

    //! add boolean option flag -key, --longkey [keytype] with description and store to dest
    void add_flag(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, bool& dest)
    {
        m_optlist.push_back(
            new argument_flag(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    //! add signed integer option -key, --longkey [keytype] with description and store to dest
    void add_int(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, int& dest)
    {
        m_optlist.push_back(
            new argument_int(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    //! add unsigned integer option -key, --longkey [keytype] with description and store to dest
    void add_uint(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, unsigned int& dest)
    {
        m_optlist.push_back(
            new argument_uint(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    //! add SI/IEC suffixes byte size option -key, --longkey [keytype] and store to 64-bit dest
    void add_bytes(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, stxxl::uint64& dest)
    {
        m_optlist.push_back(
            new argument_bytes(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    //! add string option -key, --longkey [keytype] and store to dest
    void add_string(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, std::string& dest)
    {
        m_optlist.push_back(
            new argument_string(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    //! add string list option -key, --longkey [keytype] and store to dest
    void add_stringlist(char key, const std::string& longkey, const std::string& keytype, const std::string& desc, std::vector<std::string>& dest)
    {
        m_optlist.push_back(
            new argument_stringlist(key, longkey, keytype, desc, false, dest)
            );
        calc_opt_max(m_optlist.back());
    }

    // ************************************************************************

    //! add signed integer parameter [name] with description and store to dest
    void add_param_int(const std::string& name, const std::string& desc, int& dest)
    {
        m_paramlist.push_back(
            new argument_int(0, name, "", desc, true, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add unsigned integer parameter [name] with description and store to dest
    void add_param_uint(const std::string& name, const std::string& desc, unsigned int& dest)
    {
        m_paramlist.push_back(
            new argument_uint(0, name, "", desc, true, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add SI/IEC suffixes byte size parameter [name] with description and store to dest
    void add_param_bytes(const std::string& name, const std::string& desc, uint64& dest)
    {
        m_paramlist.push_back(
            new argument_bytes(0, name, "", desc, true, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add string parameter [name] with description and store to dest
    void add_param_string(const std::string& name, const std::string& desc, std::string& dest)
    {
        m_paramlist.push_back(
            new argument_string(0, name, "", desc, true, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add string list parameter [name] with description and store to dest.
    //! \warning this parameter must be last, as it will gobble all non-option arguments!
    void add_param_stringlist(const std::string& name, const std::string& desc, std::vector<std::string>& dest)
    {
        m_paramlist.push_back(
            new argument_stringlist(0, name, "", desc, true, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    // ************************************************************************

    //! add optional signed integer parameter [name] with description and store to dest
    void add_opt_param_int(const std::string& name, const std::string& desc, int& dest)
    {
        m_paramlist.push_back(
            new argument_int(0, name, "", desc, false, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add optional unsigned integer parameter [name] with description and store to dest
    void add_opt_param_uint(const std::string& name, const std::string& desc, unsigned int& dest)
    {
        m_paramlist.push_back(
            new argument_uint(0, name, "", desc, false, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add optional SI/IEC suffixes byte size parameter [name] with description and store to dest
    void add_opt_param_bytes(const std::string& name, const std::string& desc, uint64& dest)
    {
        m_paramlist.push_back(
            new argument_bytes(0, name, "", desc, false, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add optional string parameter [name] with description and store to dest
    void add_opt_param_string(const std::string& name, const std::string& desc, std::string& dest)
    {
        m_paramlist.push_back(
            new argument_string(0, name, "", desc, false, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    //! add optional string parameter [name] with description and store to dest
    //! \warning this parameter must be last, as it will gobble all non-option arguments!
    void add_opt_param_stringlist(const std::string& name, const std::string& desc, std::vector<std::string>& dest)
    {
        m_paramlist.push_back(
            new argument_stringlist(0, name, "", desc, false, dest)
            );
        calc_param_max(m_paramlist.back());
    }

    // ************************************************************************

    //! output nicely formatted usage information including description of all
    //! parameters and options.
    void print_usage(std::ostream& os = std::cerr)
    {
        os << "Usage: " << m_progname
           << (m_optlist.size() ? " [options]" : "");

        for (arglist_type::const_iterator it = m_paramlist.begin();
             it != m_paramlist.end(); ++it)
        {
            const argument* arg = *it;

            os << (arg->m_required ? " <" : " [")
               << arg->m_longkey
               << (arg->m_repeated ? " ..." : "")
               << (arg->m_required ? '>' : ']');
        }

        os << std::endl;

        if (m_description.size())
        {
            os << std::endl;
            output_wrap(os, m_description, m_linewrap);
            os << std::endl;
        }
        if (m_author.size())
        {
            os << "Author: " << m_author << std::endl;
        }

        if (m_description.size() || m_author.size())
            os << std::endl;

        if (m_paramlist.size())
        {
            os << "Parameters:" << std::endl;

            for (arglist_type::const_iterator it = m_paramlist.begin();
                 it != m_paramlist.end(); ++it)
            {
                const argument* arg = *it;

                os << "  " << std::setw(m_param_maxlong) << std::left << arg->param_text();
                output_wrap(os, arg->m_desc, m_linewrap,
                            0, m_param_maxlong+2, m_param_maxlong+2, 8);
            }
        }

        if (m_optlist.size())
        {
            os << "Options:" << std::endl;

            for (arglist_type::const_iterator it = m_optlist.begin();
                 it != m_optlist.end(); ++it)
            {
                const argument* arg = *it;

                os << "  " << std::setw(m_opt_maxlong) << std::left << arg->option_text();
                output_wrap(os, arg->m_desc, m_linewrap,
                            0, m_opt_maxlong+2, m_opt_maxlong+2, 8);
            }
        }
    }

private:

    //! print error about option.
    void print_option_error(int argc, const char* const* argv, const argument* arg,
                            std::ostream& os)
    {
        os << (argc == 0 ? "Missing " : "Invalid ") << "argument";
        if (argc != 0)
            os << " \"" << argv[0] << "\"";

        os << " for " << arg->type_name() << " option " << arg->option_text()
           << std::endl << std::endl;

        print_usage(os);
    }

    //! print error about parameter.
    void print_param_error(int argc, const char* const* argv, const argument* arg,
                           std::ostream& os)
    {
        os << (argc == 0 ? "Missing " : "Invalid ") << "argument";
        if (argc != 0)
            os << " \"" << argv[0] << "\"";

        os << " for " << arg->type_name() << " parameter " << arg->param_text()
           << std::endl << std::endl;

        print_usage(os);
    }

public:

    //! parse command line options as specified by the options and parameters added.
    //! \return true if command line is okay and all required parameters are present.
    bool process(int argc, const char* const* argv, std::ostream& os = std::cerr)
    {
        m_progname = argv[0];
        --argc, ++argv;

        // search for help string and output help
        for (int i = 0; i < argc; ++i)
        {
            if (strcmp(argv[i], "-h") == 0 ||
                strcmp(argv[i], "--help") == 0)
            {
                print_usage(os);
                return false;
            }
        }

        arglist_type::iterator argi = m_paramlist.begin(); // current argument in m_paramlist
        bool end_optlist = false;

        while (argc != 0)
        {
            const char* arg = argv[0];

            if (arg[0] == '-' && !end_optlist)
            {
                // option, advance to argument
                --argc, ++argv;
                if (arg[1] == '-')
                {
                    if (arg[2] == '-') {
                        end_optlist = true;
                    }
                    else {
                        // long option
                        arglist_type::const_iterator oi = m_optlist.begin();
                        for ( ; oi != m_optlist.end(); ++oi)
                        {
                            if ((arg+2) == (*oi)->m_longkey)
                            {
                                if (!(*oi)->process(argc, argv))
                                {
                                    print_option_error(argc, argv, *oi, os);
                                    return false;
                                }
                                else if (m_verbose_process)
                                {
                                    os << "Option " << (*oi)->option_text() << " set to ";
                                    (*oi)->print_value(os);
                                    os << '.' << std::endl;
                                }
                                break;
                            }
                        }
                        if (oi == m_optlist.end())
                        {
                            os << "Unknown option \"" << arg << '"' << std::endl << std::endl;
                            print_usage(os);
                            return false;
                        }
                    }
                }
                else
                {
                    // short option
                    if (arg[1] == 0) {
                        os << "Invalid option \"" << arg << '"' << std::endl;
                    }
                    else {
                        arglist_type::const_iterator oi = m_optlist.begin();
                        for ( ; oi != m_optlist.end(); ++oi)
                        {
                            if (arg[1] == (*oi)->m_key)
                            {
                                if (!(*oi)->process(argc, argv))
                                {
                                    print_option_error(argc, argv, *oi, os);
                                    return false;
                                }
                                else if (m_verbose_process)
                                {
                                    os << "Option " << (*oi)->option_text() << " set to ";
                                    (*oi)->print_value(os);
                                    os << '.' << std::endl;
                                }
                                break;
                            }
                        }
                        if (oi == m_optlist.end())
                        {
                            os << "Unknown option \"" << arg << '"' << std::endl << std::endl;
                            print_usage(os);
                            return false;
                        }
                    }
                }
            }
            else
            {
                if (argi != m_paramlist.end())
                {
                    if (!(*argi)->process(argc, argv))
                    {
                        print_param_error(argc, argv, *argi, os);
                        return false;
                    }
                    else if (m_verbose_process)
                    {
                        os << "Parameter " << (*argi)->param_text() << " set to ";
                        (*argi)->print_value(os);
                        os << '.' << std::endl;
                    }
                    (*argi)->m_found = true;
                    if (!(*argi)->m_repeated)
                        ++argi;
                }
                else
                {
                    os << "Invalid extra argument " << argv[0] << std::endl << std::endl;
                    --argc, ++argv;
                    print_usage(os);
                    return false;
                }
            }
        }

        bool good = true;

        for (arglist_type::const_iterator it = m_paramlist.begin();
             it != m_paramlist.end(); ++it)
        {
            if ((*it)->m_required && !(*it)->m_found) {
                os << "Missing required argument for parameter '"
                   << (*it)->m_longkey << "'" << std::endl;
                good = false;
            }
        }

        if (!good) {
            os << std::endl;
            print_usage(os);
        }

        return good;
    }

    //! print nicely formatted result of processing
    void print_result(std::ostream& os = std::cerr)
    {
        size_t maxlong = std::max(m_param_maxlong, m_opt_maxlong);

        if (m_paramlist.size())
        {
            os << "Parameters:" << std::endl;

            for (arglist_type::const_iterator it = m_paramlist.begin();
                 it != m_paramlist.end(); ++it)
            {
                const argument* arg = *it;

                os << "  " << std::setw(maxlong) << std::left << arg->param_text();

                std::string typestr = "(" + std::string(arg->type_name()) + ")";
                os << std::setw(m_maxtypename+4) << typestr;

                arg->print_value(os);

                os << std::endl;
            }
        }

        if (m_optlist.size())
        {
            os << "Options:" << std::endl;

            for (arglist_type::const_iterator it = m_optlist.begin();
                 it != m_optlist.end(); ++it)
            {
                const argument* arg = *it;

                os << "  " << std::setw(maxlong) << std::left << arg->option_text();

                std::string typestr = "(" + std::string(arg->type_name()) + ")";
                os << std::setw(m_maxtypename+4) << std::left << typestr;

                arg->print_value(os);

                os << std::endl;
            }
        }
    }
};

__STXXL_END_NAMESPACE

#endif // STXXL_COMMON_CMDLINE_HEADER
