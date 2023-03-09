#include "hash_checker.hpp"

#include "bee/error.hpp"
#include "bee/file_path.hpp"
#include "bee/file_reader.hpp"
#include "bee/file_writer.hpp"
#include "bee/filesystem.hpp"
#include "bee/format_filesystem.hpp"
#include "bee/simple_checksum.hpp"
#include "bee/string_util.hpp"
#include "generated_build_hash.hpp"
#include "yasf/cof.hpp"

#include <map>

using bee::FilePath;
using bee::SimpleChecksum;
using bee::Time;
using std::set;
using std::string;
using std::vector;

namespace gbh = generated_build_hash;

namespace mellow {
namespace {

bee::OrError<string> hash_file(const FilePath& filename)
{
  char buffer[2048];
  bail(file, bee::FileReader::open(filename));
  SimpleChecksum h;
  while (!file->is_eof()) {
    bail(
      bytes_read,
      file->read(reinterpret_cast<std::byte*>(buffer), sizeof(buffer)));
    h.add_string(buffer, bytes_read);
  }
  return h.hex();
}

bee::OrError<gbh::TaskHash> read_task_hash(const FilePath& filename)
{
  bail(content, bee::FileReader::read_file(filename));
  return yasf::Cof::deserialize<gbh::TaskHash>(content);
}

bee::OrError<bee::Unit> write_task_hash(
  const FilePath& filename, const gbh::TaskHash& task_hash)
{
  auto content = yasf::Cof::serialize(task_hash);
  bail_unit(bee::FileSystem::mkdirs(filename.parent_path()));
  return bee::FileWriter::save_file(filename, content);
}

vector<gbh::FileHash> compute_hashes(const set<FilePath>& files)
{
  vector<gbh::FileHash> output;
  for (const auto& filename : files) {
    auto hash = hash_file(filename).move_value_default("");
    auto mtime =
      bee::FileSystem::file_mtime(filename).move_value_default(Time());
    output.push_back({
      .name = filename.to_std_path(),
      .hash = hash,
      .mtime = mtime,
    });
  }
  return output;
}

bool did_any_file_change_or_update_timestamps(
  vector<gbh::FileHash>& existing_hashes, const set<FilePath>& files)
{
  if (existing_hashes.size() != files.size()) { return true; }

  for (auto& cached : existing_hashes) {
    FilePath name = FilePath::of_string(cached.name);
    auto it = files.find(name);
    if (it == files.end()) { return true; }

    auto mtime = bee::FileSystem::file_mtime(name);
    if (mtime.is_error()) { return true; }

    if (cached.mtime == mtime.value()) {
      // If mtime is the same, we assume this file didn't change
      continue;
    }

    auto computed_hash = hash_file(name);
    if (computed_hash.is_error()) { return true; }

    if (computed_hash.value() != cached.hash) { return true; }

    // hash didn't change but mtime did, just update the mtime
    cached.mtime = mtime.value();
  }

  return false;
}

} // namespace

HashChecker::HashChecker(
  FilePath hash_filename,
  set<FilePath> inputs,
  set<FilePath> outputs,
  string non_file_inputs_key)
    : _hash_filename(std::move(hash_filename)),
      _inputs(std::move(inputs)),
      _outputs(std::move(outputs)),
      _non_file_inputs_key(std::move(non_file_inputs_key))
{}

HashChecker HashChecker::create(
  const FilePath& hash_filename,
  const set<FilePath>& inputs,
  const set<FilePath>& outputs,
  const string& non_file_inputs_key)
{
  // auto current_input_hashes = compute_hashes(inputs);

  auto current_flags_hash =
    SimpleChecksum::string_checksum(non_file_inputs_key);

  return HashChecker(hash_filename, inputs, outputs, current_flags_hash);
}

bool HashChecker::is_up_to_date()
{
  auto cached_hashes_or_error = read_task_hash(_hash_filename);

  if (cached_hashes_or_error.is_error()) { return false; }
  auto cached_hashes = std::move(cached_hashes_or_error.value());

  if (cached_hashes.flags_hash != _non_file_inputs_key) { return false; }

  if (did_any_file_change_or_update_timestamps(cached_hashes.inputs, _inputs)) {
    return false;
  }

  if (did_any_file_change_or_update_timestamps(
        cached_hashes.outputs, _outputs)) {
    return false;
  }

  _current_hashes_if_up_to_date = cached_hashes;

  return true;
}

void HashChecker::write_updated_hashes()
{
  auto get_hashes = [&]() {
    if (_current_hashes_if_up_to_date.has_value())
      return *_current_hashes_if_up_to_date;
    auto current_output_hashes = compute_hashes(_outputs);
    auto current_input_hashes = compute_hashes(_inputs);

    return gbh::TaskHash{
      .inputs = current_input_hashes,
      .outputs = current_output_hashes,
      .flags_hash = _non_file_inputs_key,
    };
  };
  auto err = write_task_hash(_hash_filename, get_hashes());

  if (err.is_error()) {
    bee::print_err_line("Failed to write hash cache for to $", _hash_filename);
  }
}

} // namespace mellow
