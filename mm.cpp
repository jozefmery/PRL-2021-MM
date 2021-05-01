/**
 * File:        mm.cpp
 * Project:     PRL-2021-Proj-2-Mesh-Multiplication
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        25.4.2021
 * Description: Mesh multiplication algorithm implementation.
 */

// std lib
#include <iostream>
#include <algorithm>
#include <regex>
#include <chrono>

// local
#include "mm.h"

using namespace MM;

vec<string> MM::split_str_by(const string& str, const string& delim) {

  std::regex re{ delim };
  return { std::sregex_token_iterator(str.begin(), str.end(), re, -1),
    std::sregex_token_iterator() };
}

Lines MM::get_lines(std::istream& is) {

  Lines out;

  for(string line; std::getline(is, line);) {

    if(line.empty()) { continue; }

    out.push_back(line);
  }

  return out;
}

Primitive MM::parse_number(const string& str) {

  std::size_t parsed{ 0 };
  int result{ 0 };
  bool failed{ false };

  try {

    result = std::stoi(str, &parsed);

    if(parsed != str.size()) {
      
      failed = true;
    }

  } catch(const std::invalid_argument&) { 

    failed = true;
  }
  
  if(failed) {

    throw Abort{ "Invalid number: " + str, ExitCode::INPUT_ERROR };
  }

  return result;
}

Matrix::Matrix(const MatrixFile& file) {

  std::ifstream handle { file.name };

  if(!handle.good()) {

    stream s;
    s << "The " << file.name << " input file was not found in the application directory";

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  file_ = ReadMatrixFile{ file.name, file.contained_dim, get_lines(handle) };

  read_matrix();
}

Matrix::Matrix(const MatrixDimensions& dim) {

  resize(dim);
}

void Matrix::print() const {

  std::cout << rows() << ":" << cols() << "\n";
  
  for(const auto& row : data_) {

    // print all items but last with space delimiter
    std::copy(row.begin(), row.end() - 1, std::ostream_iterator<int>{ std::cout, " " });
    // print last item without a delimiter after it, and add a newline
    std::cout << *(row.end() - 1) << "\n";
  }
}

Primitive Matrix::get(const MatrixPos& pos) const {

  if(pos.row >= rows() || pos.col >= cols()) {

    throw Abort{ "Matrix get out of bounds", ExitCode::MAT_OP_ERROR };
  }

  return data_[pos.row][pos.col];
}

void Matrix::set(const MatrixPos& pos, const Primitive value) {

  if(pos.row >= rows() || pos.col >= cols()) {

    throw Abort{ "Matrix get out of bounds", ExitCode::MAT_OP_ERROR };
  }

  data_[pos.row][pos.col] = value;
}

void Matrix::resize(const MatrixDimensions& dim) {
  
  for(size_t row = 0; row < dim.rows; ++row) {

    data_.emplace_back(dim.cols, 0);
  }

  dim_ = dim;
}

void Matrix::check_file_not_empty() const {

  if(file_.lines.empty()) {

    stream s;
    s << "The " << file_.name << " is empty";

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }
}

void Matrix::check_consistent_rows() const {

  // there is at least one item
  auto cols{ data_[0].size() };

  // make sure all rows have the same number of items
  for(size_t i{ 1 }; i < data_.size(); ++i) {

    if(data_[i].size() != cols) {

      stream s;
      s << "Inconsistent matrix rows in " << file_.name;

      throw Abort{ s.str(), ExitCode::INPUT_ERROR };
    }
  }
}

void Matrix::check_matrix_not_empty() const {

  if(data_.empty()) {

    stream s;
    s << "No matrix data in " << file_.name;

    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }
}

Primitive Matrix::read_dimension() const {

  // shorthand
  const auto& lines{ file_.lines };

  Primitive dim_value{ -1 };

  try {

    // there is at least one item
    dim_value = parse_number(lines[0]);
  
  } catch(const Abort&) {

    stream s;
    s << "Invalid dimension \"" << lines[0] << "\" in " << file_.name;
    // rethrow more specific message
    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  if(dim_value < 1) {

    stream s;
    s << "Dimension value in " << file_.name << " is less than 1";
    // rethrow more specific message
    throw Abort{ s.str(), ExitCode::INPUT_ERROR };
  }

  return dim_value;
}

vec<Primitive> Matrix::str_to_row(const string& str) const {

  // split by " " and then filter empty strings
  const auto str_nums{ vec_filter(split_str_by(str, " "), string{ "" }) };
  vec<Primitive> nums;
  nums.resize(str_nums.size());

  try {

    std::transform(str_nums.begin(), str_nums.end(), nums.begin(), parse_number);
  
  } catch(const Abort& a) {

    stream s;
    s << a.message << " in " << file_.name;

    throw Abort{ s.str(), a.code };
  }

  return nums;
}

void Matrix::check_contained_dim() const {

  const auto dim{ read_dimension() };
  
  switch(file_.contained_dim) {

    case MatrixFileDimension::ROWS:

      if(rows() != dim) {

        stream s;
        s << "Unexpected number of rows in " << file_.name;

        throw Abort{ s.str(), ExitCode::INPUT_ERROR };
      }

      break;

    case MatrixFileDimension::COLS:

      if(cols() != dim) {

        stream s;
        s << "Unexpected number of columns in " << file_.name;

        throw Abort{ s.str(), ExitCode::INPUT_ERROR };
      }

      break;
  }
}

void Matrix::read_matrix() {

  // shorthand
  const auto& lines{ file_.lines };

  check_file_not_empty();

  // do not reserve space for the dimension item
  data_.resize(lines.size() - 1);

  std::transform(lines.begin() + 1, lines.end(), data_.begin(), [&](const string& s) { return str_to_row(s); });

  dim_ = { data_.size(), data_[0].size() };

  check_matrix_not_empty();
  check_consistent_rows();
  check_contained_dim();
}

Process::Base::Base(const Pid pid, const int p_count) 

  : pid_{ pid }
  , p_count_{ p_count }
{}

string Process::Base::format_error(const string& message) const {

  stream s;

  s << "[ERROR in " << pid_ << "]: " << message << "\n";

  return s.str();
}

Process::Enumerator::Enumerator(const Pid pid, const int p_count) 

  : Base{ pid, p_count }
  , message_{ 0, 0 }
  , accumulator_{ 0 }
  , dim_{ 0, 0 }
  , shared_{ 0 }
{}

void Process::Enumerator::run() {

  recv_dim();
  enumerate();
}

Pid Process::Enumerator::pid_up() const {

  if(first_row()) {

    return MAIN_PROCESS;
  }

  return pid_ - dim_.cols;
}

Pid Process::Enumerator::pid_right() const {

  return pid_ + 1;
}

Pid Process::Enumerator::pid_down() const {

  return pid_ + dim_.cols;
}

Pid Process::Enumerator::pid_left() const {

  if(first_col()) {

    return MAIN_PROCESS;
  }

  return pid_ - 1;
}

bool Process::Enumerator::first_row() const {

  return pid_ < dim_.cols;
}

bool Process::Enumerator::last_row() const {

  return pid_ >= (dim_.rows - 1) * dim_.cols; 
}

bool Process::Enumerator::first_col() const {

  return pid_ % dim_.cols == 0;
}

bool Process::Enumerator::last_col() const {

  return (pid_ + 1) % dim_.cols == 0;
}

Primitive Process::Enumerator::recv(const Pid source, const Tag tag) const {

  Primitive value;
  MPI_Status status;

  MPI_Recv(&value, 1, MPI_INT, source, static_cast<int>(tag), MPI_COMM_WORLD, &status);
    
  if(status.MPI_ERROR) {
    
    stream s;
    s << "MPI_Recv error: " << status.MPI_ERROR;

    throw Abort{ s.str(), ExitCode::MPI_ERROR }; 
  }

  return value;
}

void Process::Enumerator::send(const Pid target, const Primitive value, const Tag tag) const {

  MPI_Send(const_cast<Primitive*>(&value), 1, MPI_INT, target, static_cast<int>(tag), MPI_COMM_WORLD);
}

void Process::Enumerator::recv_dim() {

  size_t values[3];
  MPI_Bcast(values, sizeof(values) / sizeof(size_t), MPI_UNSIGNED_LONG_LONG, MAIN_PROCESS, MPI_COMM_WORLD);

  dim_.rows = values[0];
  dim_.cols = values[1];
  shared_   = values[2];
}

void Process::Enumerator::enumerate() {

  for (size_t i{ 0 }; i < shared_; ++i) {
    
    recv_message();
    accumulate();
    propagate();
  }
  send_result();
}

void Process::Enumerator::recv_message() {

  message_.left = recv(pid_left(), Tag::LEFT);
  message_.up   = recv(pid_up(), Tag::UP);
}

void Process::Enumerator::accumulate() {

  accumulator_ += message_.left * message_.up;
}

void Process::Enumerator::propagate() {

  if (!last_col()) {

    send(pid_right(), message_.left, Tag::LEFT);
  }

  if (!last_row()) {

    send(pid_down(), message_.up, Tag::UP);
  }
}

void Process::Enumerator::send_result() {

  send(MAIN_PROCESS, accumulator_, Tag::NONE);
}

Process::Main::Main(const Pid pid, const int p_count) 
  
  : Enumerator{ pid, p_count }
  , input_{ Matrix{ MAT1 }, Matrix{ MAT2 } }
  , result_{ MatrixDimensions{ 0, 0 } }
{
  check_input();
  check_processes();

  dim_    = MatrixDimensions{ input_[0].rows(), input_[1].cols() };
  shared_ = input_[0].cols();

  result_.resize(dim_);
}

void Process::Main::run() {

  using std::chrono::high_resolution_clock;
  using std::chrono::duration;
  using std::chrono::milliseconds;

  const auto t1 = high_resolution_clock::now();

  send_dim();
  propagate_matrix();
  enumerate();
  recv_result();

  const auto t2 = high_resolution_clock::now();

  // getting number of milliseconds as a double
  const duration<double, std::milli> diff = t2 - t1;

  if(BENCHMARK) {

    std::cout << diff.count();

  } else {

    result_.print();
  }
}

void Process::Main::check_input() const {

  const auto cols{ input_[0].cols() };
  const auto rows{ input_[1].rows() };

  if(rows != cols) {

    throw Abort{"Incompatible matrix dimensions for multiplication", ExitCode::INPUT_ERROR };
  }
}

void Process::Main::check_processes() const {

  const auto rows{ input_[0].rows() };
  const auto cols{ input_[1].cols() };

  if(rows * cols != p_count_) {

    throw Abort{ "The required number of processes was not launched", ExitCode::MPI_ERROR };
  }
}

void Process::Main::send_dim() const {

  size_t values[]{ dim_.rows, dim_.cols, shared_ };
  MPI_Bcast(values, sizeof(values) / sizeof(size_t), MPI_UNSIGNED_LONG_LONG, pid_, MPI_COMM_WORLD);
}

void Process::Main::recv_result() {

  for(int i{ 0 }; i < p_count_; ++i) {

    result_.set({ i / dim_.cols, i % dim_.cols }, recv(i, Tag::ANY));
  }
}

void Process::Main::propagate_matrix() const {

  for (size_t i{ 0 }; i < shared_; ++i) {

    for(size_t row{ 0 }; row < dim_.rows; ++row) {
      
      send(row * dim_.cols, input_[0].get({ row, i }), Tag::LEFT);
    }
    
    for(size_t col{ 0 }; col < dim_.cols; ++col) {

      send(col, input_[1].get({ i, col }), Tag::UP);
    }
  }
}

/* static */ SpecificProcess Application::get_process(const Pid pid, const int p_count) {

  if(pid == MAIN_PROCESS) {

    return new Process::Main{ pid, p_count };
  }
  
  return new Process::Enumerator{ pid, p_count };
}

Application::Application(const int argc, const char* const argv[]) {
  
  // values aren't changed but passed as C-style references
  MPI_Init(const_cast<int*>(&argc), const_cast<char***>(&argv));

  // initialize to invalid values
  int p_count { 0 };
  Pid pid     { -1 };
 
  // fetch process count and current process id
  MPI_Comm_size(MPI_COMM_WORLD, &p_count);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  process_ = get_process(pid, p_count);
}

Application::~Application() noexcept {

  MPI_Finalize();
  delete process_;
}

void Application::run() {

  process_->run();
}

int main(int argc, char* argv[]) {

  try {

    MM::Application{ argc, argv }.run();
  
  } catch(const Abort& abort) {

    std::cerr << abort.message;
    MPI_Abort(MPI_COMM_WORLD, static_cast<int>(abort.code));
  }
  
  return static_cast<int>(ExitCode::OK);
}