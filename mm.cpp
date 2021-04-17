/**
 * File:        mm.cpp
 * Project:     PRL-2021-Proj-2-Mesh-Multiplication
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        15.4.2021
 * Description: TODO
 */

// std lib
#include <iostream>
#include <algorithm>
#include <regex>

// OpenMPI
#include <mpi.h>

// local
#include "mm.h"

using namespace MM;

vec<string> MM::split_str_by(const string& str, const string& delim) {

  std::regex re{ delim };
  return { std::sregex_token_iterator(str.begin(), str.end(), re, -1),
    std::sregex_token_iterator() };
}

Process::Base::Base(const Pid pid, const int p_count) 

  : pid_{ pid }
  , p_count_{ p_count }
{}

string Process::Base::format_error(const string& message) const {

  ss stream;

  stream << "[ERROR in " << pid_ << "]: " << message << "\n";

  return stream.str();
}

void Process::Base::abort(const string& message, const ExitCode exit_code) const {

    std::cerr << format_error(message);
    MPI_Abort(MPI_COMM_WORLD, static_cast<int>(exit_code));
}

Process::Main::Main(const Pid pid, const int p_count) 
  
  : Enumerator{ pid, p_count }
{
  read_input();
  check_input();
  check_processes();
}

/* static */ Lines Process::Main::get_lines(std::istream& is) {

  Lines out;

  for(string line; std::getline(is, line);) {

    if(line.empty()) { continue; }

    out.push_back(line);
  }

  return out;
}

/* static */ bool Process::Main::parse_number(const string& str, Primitive& result) {

  std::size_t parsed{ 0 };

  try {

    result = std::stoi(str, &parsed);

    return parsed == str.size();

  } catch(const std::invalid_argument&) { /* pass */ }
  
  return false;
}

Matrix Process::Main::read_file_to_matrix(const ReadFile& file) const {

  // shorthand
  auto& lines{ file.lines };

  if(lines.empty()) {

    ss s;
    s << "The " << file.name << " is empty";

    abort(s.str(), ExitCode::INPUT_ERROR);
  }

  Primitive dim_value{ -1 };

  // there is at least one item
  if(!parse_number(lines[0], dim_value)) {
    
    ss s;
    s << "Invalid dimension \"" << lines[0] << "\" in " << file.name;

    abort(s.str(), ExitCode::INPUT_ERROR);
  }

  if(dim_value < 1) {

    ss s;
    s << "Invalid dimension value \"" << lines[0] << "\" in " << file.name;

    abort(s.str(), ExitCode::INPUT_ERROR);
  }

  // do not reserve space for the dimension item
  Matrix matrix{ lines.size() - 1 };

  const auto str_to_numbers{ [&](const string& str) {

    // split by " " and then filter empty strings
    const auto num_vec{ vec_filter(split_str_by(str, " "), string{ "" }) };
    vec<Primitive> out;
    out.reserve(num_vec.size());

    for(const auto num_str : num_vec) {

      Primitive num;

      if(!parse_number(num_str, num)) {

        ss s;
        s << "Invalid number \"" << num_str << "\" in " << file.name;

        abort(s.str(), ExitCode::INPUT_ERROR);
      }

      out.push_back(num);
    }

    return out;

  } };

  std::transform(lines.begin() + 1, lines.end(), matrix.begin(), str_to_numbers);

  if(matrix.empty()) {

    ss s;
    s << "No matrix data in " << file.name;

    abort(s.str(), ExitCode::INPUT_ERROR);
  }

  // there is at least one item
  auto cols{ matrix[0].size() };

  // make sure all rows have the same number of items
  for(size_t i{ 1 }; i < matrix.size(); ++i) {

    if(matrix[i].size() != cols) {

      ss s;
      s << "Inconsistent matrix columns in " << file.name;

      abort(s.str(), ExitCode::INPUT_ERROR);
    }
  }
  
  switch(file.contained_dim) {

    case MatrixDimension::ROWS:

      if(matrix.size() != dim_value) {

        ss s;
        s << "Unexpected number of rows in " << file.name;

        abort(s.str(), ExitCode::INPUT_ERROR);
      }

      break;

    case MatrixDimension::COLS:

      if(cols != dim_value) {

        ss s;
        s << "Unexpected number of columns in " << file.name;

        abort(s.str(), ExitCode::INPUT_ERROR);
      }

      break;
  }

  return matrix;
}

Matrix Process::Main::read_matrix(const InputFile& file) const {
  
  std::ifstream handle { file.name };

  if(!handle.good()) {

    ss s;
    s << "The " << file.name << " input file was not found in the application directory";

    abort(s.str(), ExitCode::INPUT_ERROR);
  }

  return read_file_to_matrix({ file.name, file.contained_dim, get_lines(handle) });
}

void Process::Main::read_input() {

  for(int i{ 0 }; i < input_.size(); ++i) {

    input_[i] = read_matrix(INPUTS[i]);
  }
}

void Process::Main::check_input() const {

  const auto rows = input_[0][0].size();
  const auto cols = input_[1].size();


  if(rows != cols) {

    abort("Incompatible matrix dimensions for multiplication", ExitCode::INPUT_ERROR);
  }
}

void Process::Main::check_processes() const {

  const auto rows = input_[0].size();
  const auto cols = input_[1][0].size();;

  if(rows * cols != p_count_) {

    abort("The required number of processes was not launched", ExitCode::MPI_ERROR);
  }
}

void Process::Main::run() {

  Enumerator::run();
}

Process::Enumerator::Enumerator(const Pid pid, const int p_count) 

  : Base{ pid, p_count }

{
  // TODO 
}

void Process::Enumerator::run() {

  // TODO
}

/* static */ SpecificProcess Application::get_process(const Pid pid, const int p_count) {

  if(pid == MAIN_PROCESS) {

    return std::make_unique<Process::Main>(pid, p_count);
  }
  
  return std::make_unique<Process::Enumerator>(pid, p_count);
}

Application::Application(const int argc, const char* const argv[]) {
  
  // values aren't changed but passed as C-style references
  MPI_Init(const_cast<int*>(&argc), const_cast<char***>(&argv));

  // initialize to invalid values
  int p_count { 0 };
  Pid pid   { -1 };
 
  // fetch process count and current process id
  MPI_Comm_size(MPI_COMM_WORLD, &p_count);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  process_ = get_process(pid, p_count);
}

Application::~Application() noexcept {

  MPI_Finalize();
}

void Application::run() {

  process_->run();
}

int main(int argc, char* argv[]) {

  MM::Application{ argc, argv }.run();
  return static_cast<int>(ExitCode::OK);
}