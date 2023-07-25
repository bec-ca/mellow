#pragma once

#include <set>
#include <string>

#include "package_path.hpp"

#include "bee/util.hpp"
#include "yasf/location.hpp"

namespace mellow {
namespace rules {
namespace details {

template <class T>
concept HasNameField = requires(T t) { t.name; };

template <class T>
concept HasLibsField = requires(T t) { t.libs; };

template <class T>
concept HasHeadersField = requires(T t) { t.headers; };

template <class T>
concept HasSourcesField = requires(T t) { t.sources; };

template <class T>
concept HasLdFlagsField = requires(T t) { t.ld_flags; };

template <class T>
concept HasOutputCppObjectsFn = requires(T t) { t.output_cpp_object(); };

template <class T>
concept HasOutputs = requires(T t) { t.outputs; };

template <class T>
concept HasOutput = requires(T t) { t.output; };

template <class T>
concept HasBinary = requires(T t) { t.binary; };

template <class T>
concept HasSystemLibConfig = requires(T t) { t.system_lib_config(); };

template <class T>
concept HasLocation = requires(T t) { t.location; };

////////////////////////////////////////////////////////////////////////////////
// ProvideLibs
//

template <class T, class P> struct ProvideLibs;

template <HasLibsField T, class P> struct ProvideLibs<T, P> {
 public:
  std::set<PackagePath> libs() const
  {
    const auto& p = _p();
    return p.to_package(p.raw().libs);
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideLibs {};

////////////////////////////////////////////////////////////////////////////////
// ProvideHeaders
//

template <class T, class P> struct ProvideHeaders;

template <HasHeadersField T, class P> struct ProvideHeaders<T, P> {
 public:
  std::set<std::string> headers() const
  {
    auto& h = _p().raw().headers;
    return std::set(h.begin(), h.end());
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideHeaders {};

////////////////////////////////////////////////////////////////////////////////
// ProvideSources
//

template <class T, class P> struct ProvideSources;

template <HasSourcesField T, class P> struct ProvideSources<T, P> {
 public:
  std::set<std::string> sources() const
  {
    const auto& p = _p();
    return bee::to_set(p.raw().sources);
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideSources {};

////////////////////////////////////////////////////////////////////////////////
// ProvideLdFlags
//

template <class T, class P> struct ProvideLdFlags;

template <HasLdFlagsField T, class P> struct ProvideLdFlags<T, P> {
 public:
  const std::vector<std::string>& ld_flags() const
  {
    return _p().raw().ld_flags;
  }

  std::vector<std::string>& ld_flags() { return _p().raw().ld_flags; }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
  P& _p() { return static_cast<P&>(*this); }
};

template <class T, class P> struct ProvideLdFlags {};

////////////////////////////////////////////////////////////////////////////////
// ProvideOutputCppObject
//

template <class T, class P> struct ProvideOutputCppObject;

template <HasOutputCppObjectsFn T, class P>
struct ProvideOutputCppObject<T, P> {
 public:
  std::optional<PackagePath> output_cpp_object() const
  {
    const auto& p = _p();
    return p.to_package(p.raw().output_cpp_object());
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideOutputCppObject {};

////////////////////////////////////////////////////////////////////////////////
// ProvideOutputs
//

template <class T, class P> struct ProvideOutputs;

template <HasOutputs T, class P> struct ProvideOutputs<T, P> {
 public:
  std::set<PackagePath> outputs() const
  {
    const auto& p = _p();
    return p.to_package(p.raw().outputs);
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideOutputs {};

////////////////////////////////////////////////////////////////////////////////
// ProvideOutput
//

template <class T, class P> struct ProvideOutput;

template <HasOutput T, class P> struct ProvideOutput<T, P> {
 public:
  PackagePath output() const
  {
    const auto& p = _p();
    return p.to_package(p.raw().output);
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
};

template <class T, class P> struct ProvideOutput {};

////////////////////////////////////////////////////////////////////////////////
// ProvideBinary
//

template <class T, class P> struct ProvideBinary;

template <HasBinary T, class P> struct ProvideBinary<T, P> {
 public:
  PackagePath binary() const
  {
    const auto& p = _p();
    return p.package_path() / p.raw().binary;
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
  P& _p() { return static_cast<P&>(*this); }
};

template <class T, class P> struct ProvideBinary {};

////////////////////////////////////////////////////////////////////////////////
// ProvideSystemLibConfig
//

template <class T, class P> struct ProvideSystemLibConfig;

template <HasSystemLibConfig T, class P> struct ProvideSystemLibConfig<T, P> {
 public:
  PackagePath system_lib_config() const
  {
    const auto& p = _p();
    return p.package_path() / p.raw().system_lib_config();
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
  P& _p() { return static_cast<P&>(*this); }
};

template <class T, class P> struct ProvideSystemLibConfig {};

////////////////////////////////////////////////////////////////////////////////
// ProvideLocation
//

template <class T, class P> struct ProvideLocation;

template <HasLocation T, class P> struct ProvideLocation<T, P> {
 public:
  const std::optional<yasf::Location>& location() const
  {
    const auto& p = _p();
    return p.raw().location;
  }

 private:
  const P& _p() const { return static_cast<const P&>(*this); }
  P& _p() { return static_cast<P&>(*this); }
};

template <class T, class P> struct ProvideLocation {};

} // namespace details

template <details::HasNameField T>
struct BaseRule : public details::ProvideLibs<T, BaseRule<T>>,
                  public details::ProvideHeaders<T, BaseRule<T>>,
                  public details::ProvideSources<T, BaseRule<T>>,
                  public details::ProvideLdFlags<T, BaseRule<T>>,
                  public details::ProvideOutputCppObject<T, BaseRule<T>>,
                  public details::ProvideOutputs<T, BaseRule<T>>,
                  public details::ProvideOutput<T, BaseRule<T>>,
                  public details::ProvideBinary<T, BaseRule<T>>,
                  public details::ProvideSystemLibConfig<T, BaseRule<T>>,
                  public details::ProvideLocation<T, BaseRule<T>> {
 public:
  using format_type = T;

  BaseRule(const T& f, const PackagePath& path)
      : _format(f), _package_path(path)
  {}

  const T& raw() const { return _format; }
  T& raw() { return _format; }

  PackagePath name() const { return _package_path / _format.name; }

  const PackagePath& package_path() const { return _package_path; }

  PackagePath to_package(const std::string& path) const
  {
    return _package_path / path;
  }

  std::set<PackagePath> to_package(const std::vector<std::string>& paths) const
  {
    std::set<PackagePath> output;
    for (const auto& path : paths) { output.insert(_package_path / path); }
    return output;
  }

  std::optional<PackagePath> to_package(
    const std::optional<std::string>& path) const
  {
    if (!path.has_value()) return std::nullopt;
    return _package_path / *path;
  }

 private:
  T _format;
  PackagePath _package_path;
};

} // namespace rules
} // namespace mellow
