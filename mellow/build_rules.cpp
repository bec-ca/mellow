#include "build_rules.hpp"

#include "generated_mbuild_parser.hpp"
#include "package_path.hpp"

#include "bee/util.hpp"
#include "yasf/cof.hpp"

using std::decay_t;
using std::is_same_v;
using std::nullopt;
using std::optional;
using std::set;
using std::string;
using std::vector;

namespace mellow {

namespace rules {

namespace {

Rule::rule_variant make_rule_variant(
  const Rule::format_variant& rs, const PackagePath& path)
{
  return visit(
    [&path](auto& rule) -> Rule::rule_variant {
      using T = decay_t<decltype(rule)>;
      if constexpr (is_same_v<T, gmp::CppBinary>) {
        return CppBinary(rule, path);
      } else if constexpr (is_same_v<T, gmp::CppLibrary>) {
        return CppLibrary(rule, path);
      } else if constexpr (is_same_v<T, gmp::CppTest>) {
        return CppTest(rule, path);
      } else if constexpr (is_same_v<T, gmp::GenRule>) {
        return GenRule(rule, path);
      } else if constexpr (is_same_v<T, gmp::SystemLib>) {
        return SystemLib(rule, path);
      } else {
        static_assert(bee::always_false_v<T> && "non exaustive visit");
      }
    },
    rs);
}

template <class T>
concept HasSources = requires(T t) {
                       {
                         t.sources()
                       };
                     };

template <class T>
concept HasLibs = requires(T t) {
                    {
                      t.libs()
                    };
                  };

template <class T>
concept HasHeaders = requires(T t) {
                       {
                         t.headers()
                       };
                     };

template <class T>
concept HasLdFlags = requires(T t) {
                       {
                         t.ld_flags
                       };
                     };

template <class T>
concept HasCppFlags = requires(T t) {
                        {
                          t.cpp_flags
                        };
                      };

template <class T>
concept HasAdditionalDeps = requires(T t) {
                              {
                                t.additional_deps()
                              };
                            };

template <class T>
concept HasOutputCppObjects = requires(T t) {
                                {
                                  t.output_cpp_object()
                                };
                              };

template <class T>
concept HasName = requires(T t) {
                    {
                      t.name()
                      } -> std::convertible_to<PackagePath>;
                  };

template <class T>
concept HasOutputs = requires(T t) { t.outputs(); };

template <class T>
concept HasOsFilter = requires(T t) { t.os_filter(); };

} // namespace

////////////////////////////////////////////////////////////////////////////////
// CppBinary
//

CppBinary::CppBinary(const gmp::CppBinary& p, const PackagePath& path)
    : BaseRule(p, path)
{}

CppBinary::~CppBinary() {}

static_assert(HasName<CppBinary>);
static_assert(HasLibs<CppBinary>);
static_assert(HasSources<CppBinary>);

////////////////////////////////////////////////////////////////////////////////
// CppLibrary
//

namespace details {

CppLibraryBase::CppLibraryBase(const gmp::CppLibrary& format)
    : CppLibrary(format)
{}

optional<string> CppLibraryBase::output_cpp_object() const
{
  if (sources.empty()) { return nullopt; }
  return {name + ".o"};
}

} // namespace details

CppLibrary::CppLibrary(const gmp::CppLibrary& p, const PackagePath& path)
    : BaseRule(p, path)
{}

CppLibrary::~CppLibrary() {}

static_assert(HasOutputCppObjects<CppLibrary>);
static_assert(HasName<CppLibrary>);
static_assert(HasLibs<CppLibrary>);
static_assert(HasHeaders<CppLibrary>);
static_assert(HasSources<CppLibrary>);

////////////////////////////////////////////////////////////////////////////////
// CppTest
//

CppTest::CppTest(const gmp::CppTest& p, const PackagePath& path)
    : BaseRule(p, path)
{}

CppTest::~CppTest() {}

const std::vector<std::string>& CppTest::os_filter() const
{
  return raw().os_filter;
}

std::vector<std::string>& CppTest::os_filter() { return raw().os_filter; }

static_assert(HasName<CppTest>);

////////////////////////////////////////////////////////////////////////////////
// GenRule
//

GenRule::GenRule(const gmp::GenRule& p, const PackagePath& path)
    : BaseRule(p, path)
{}

GenRule::~GenRule() {}

set<string> GenRule::additional_deps() const { return {raw().binary}; }

static_assert(HasName<GenRule>);
static_assert(HasOutputs<GenRule>);

////////////////////////////////////////////////////////////////////////////////
// SystemLib
//

namespace details {

SystemLibBase::SystemLibBase(const gmp::SystemLib& format) : SystemLib(format)
{}

string SystemLibBase::system_lib_config() const { return name + ".output"; }

} // namespace details
SystemLib::SystemLib(const gmp::SystemLib& p, const PackagePath& path)
    : BaseRule(p, path)
{}

SystemLib::~SystemLib() {}

vector<string> SystemLib::provide_headers() const
{
  return raw().provide_headers;
}

static_assert(HasName<SystemLib>);
static_assert(details::HasSystemLibConfig<SystemLib>);

////////////////////////////////////////////////////////////////////////////////
// ExternalPackage
//

ExternalPackage::ExternalPackage(const gmp::ExternalPackage& format)
    : BaseRule(format, PackagePath::root())
{}

ExternalPackage::~ExternalPackage() {}

////////////////////////////////////////////////////////////////////////////////
// Rule
//

Rule::Rule(const Rule::format_variant& r, const PackagePath& path)
    : rule(make_rule_variant(r, path))
{}

Rule::~Rule() {}

gmp::Rule Rule::to_format() const
{
  return gmp::Rule(visit(
    []<class T>(const T& rule) -> gmp::Rule::value_type { return rule.raw(); },
    rule));
}

const PackagePath& Rule::package_path() const
{
  return visit(
    []<class T>(const T& rule) -> const PackagePath& {
      return rule.package_path();
    },
    rule);
}

PackagePath Rule::name() const
{
  return visit(
    []<class T>(const T& rule) {
      if constexpr (HasName<T>) {
        return rule.name();
      } else {
        return rule.name;
      }
    },
    rule);
}

set<string> Rule::sources() const
{
  return visit(
    []<class T>(const T& rule) -> set<string> {
      if constexpr (HasSources<T>) {
        return rule.sources();
      } else {
        return {};
      }
    },
    rule);
}

set<string> Rule::headers() const
{
  return visit(
    []<class T>(const T& rule) -> set<string> {
      if constexpr (HasHeaders<T>) {
        return rule.headers();
      } else {
        return {};
      }
    },
    rule);
}

set<PackagePath> Rule::libs() const
{
  return visit(
    []<class T>(const T& rule) -> set<PackagePath> {
      if constexpr (HasLibs<T>) {
        return bee::to_set(rule.libs());
      } else {
        return {};
      }
    },
    rule);
}

set<PackagePath> Rule::deps() const
{
  auto additional_deps = visit(
    []<class T>(const T& rule) -> set<PackagePath> {
      if constexpr (std::is_same_v<T, GenRule>) {
        return {rule.binary()};
      } else {
        return {};
      }
    },
    rule);
  return bee::compose_set(libs(), additional_deps);
}

vector<string> Rule::ld_flags() const
{
  return visit(
    []<class T>(const T& rule) -> vector<string> {
      if constexpr (HasLdFlags<T>) {
        return rule.ld_flags;
      } else {
        return {};
      }
    },
    rule);
}

vector<string> Rule::cpp_flags() const
{
  return visit(
    []<class T>(const T& rule) -> vector<string> {
      if constexpr (HasCppFlags<T>) {
        return rule.cpp_flags;
      } else {
        return {};
      }
    },
    rule);
}

optional<PackagePath> Rule::output_cpp_object() const
{
  return visit(
    []<class T>(const T& rule) -> optional<PackagePath> {
      if constexpr (HasOutputCppObjects<T>) {
        return rule.output_cpp_object();
      } else {
        return nullopt;
      }
    },
    rule);
}

optional<PackagePath> Rule::system_lib_config() const
{
  return visit(
    []<class T>(const T& rule) -> optional<PackagePath> {
      if constexpr (details::HasSystemLibConfig<T>) {
        return rule.system_lib_config();
      } else {
        return nullopt;
      }
    },
    rule);
}

vector<string> Rule::os_filter() const
{
  return visit(
    []<class T>(const T& rule) -> vector<string> {
      if constexpr (HasOsFilter<T>) {
        return rule.os_filter();
      } else {
        return {};
      }
    },
    rule);
}

const optional<yasf::Location>& Rule::location() const
{
  return visit(
    []<class T>(const T& rule) -> const optional<yasf::Location>& {
      return rule.location();
    },
    rule);
}

} // namespace rules

} // namespace mellow
