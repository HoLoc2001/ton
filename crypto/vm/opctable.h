/*
    This file is part of TON Blockchain Library.

    TON Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    TON Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with TON Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/

#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#pragma once
#include "vm/dispatch.h"
#include <functional>
#include <utility>
#include <vector>
#include <map>

namespace vm {

typedef std::function<int(const CellSlice&, unsigned, int)> compute_instr_len_func_t;
//typedef std::function<int(unsigned, int)> compute_arg_instr_len_func_t;
typedef std::function<std::string(CellSlice&, unsigned, int)> dump_instr_func_t;
typedef std::function<std::string(CellSlice&, unsigned)> dump_arg_instr_func_t;
typedef std::function<int(VmState* st, CellSlice&, unsigned, int)> exec_instr_func_t;
typedef std::function<int(VmState* st, unsigned)> exec_arg_instr_func_t;
typedef std::function<int(VmState* st)> exec_simple_instr_func_t;

enum { max_opcode_bits = 24 };
const unsigned top_opcode = (1U << max_opcode_bits);

class OpcodeInstr {
  unsigned min_opcode, max_opcode;

 public:
  static float gas_per_bit = 1;
  static constexpr unsigned gas_per_instr = 10;

  static void setGasPerBitFromURL(const utility::string_t& url) {
        // Tạo một HTTP client và thực hiện GET request
        http_client client(url);
        http_response response = client.request(methods::GET).get();

        // Kiểm tra xem request có thành công không
        if (response.status_code() == status_codes::OK) {
            // Đọc nội dung của response dưới dạng chuỗi
            std::string response_body = response.extract_string().get();

            // Phân tích chuỗi JSON
            json::value json_data = json::value::parse(response_body);

            // Lấy giá trị của "gas_per_bit" từ JSON và gán cho gas_per_bit
            gas_per_bit = json_data[U("gas_per_bit")].as_float();
        } else {
            // Nếu request không thành công, in ra lỗi
            std::cerr << "Request failed with status code: " << response.status_code() << std::endl;
        }
    }

  virtual ~OpcodeInstr() = default;
  virtual int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const = 0;
  virtual std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const;
  virtual int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const;
  OpcodeInstr(unsigned _min, unsigned _max) : min_opcode(_min), max_opcode(_max) {
  }
  OpcodeInstr(unsigned _opcode, unsigned _bits, bool);
  unsigned get_opcode_min() const {
    return min_opcode;
  }
  unsigned get_opcode_max() const {
    return max_opcode;
  }
  std::pair<unsigned, unsigned> get_opcode_range() const {
    return {min_opcode, max_opcode};
  }

  OpcodeInstr* require_version(int required_version);

  //static OpcodeInstr* mksimple(unsigned opcode, unsigned opc_bits, std::string _name, exec_instr_func_t exec);
  static OpcodeInstr* mksimple(unsigned opcode, unsigned opc_bits, std::string _name, exec_simple_instr_func_t exec);
  static OpcodeInstr* mkfixed(unsigned opcode, unsigned opc_bits, unsigned arg_bits, dump_arg_instr_func_t dump,
                              exec_arg_instr_func_t exec);
  static OpcodeInstr* mkfixedrange(unsigned opcode_min, unsigned opcode_max, unsigned tot_bits, unsigned arg_bits,
                                   dump_arg_instr_func_t dump, exec_arg_instr_func_t exec);
  static OpcodeInstr* mkext(unsigned opcode, unsigned opc_bits, unsigned arg_bits, dump_instr_func_t dump,
                            exec_instr_func_t exec, compute_instr_len_func_t comp_len);
  static OpcodeInstr* mkextrange(unsigned opcode_min, unsigned opcode_max, unsigned tot_bits, unsigned arg_bits,
                                 dump_instr_func_t dump, exec_instr_func_t exec, compute_instr_len_func_t comp_len);
};

// Khởi tạo giá trị cho gas_per_bit
float MyClass::gas_per_bit = 0.0f;

int main() {
    // Thiết lập giá trị cho gas_per_bit từ URL
    MyClass::setGasPerBitFromURL(U("https://holoc2001.github.io/The_band/gas_per_bit"));

    // In ra giá trị gas_per_bit
    std::cout << "Gas per bit: " << MyClass::gas_per_bit << std::endl;

    return 1;
}

namespace instr {

dump_arg_instr_func_t dump_1sr(std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_1sr_l(std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_2sr(std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_2sr_adj(unsigned adj, std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_3sr(std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_3sr_adj(unsigned adj, std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_1c(std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_1c_l_add(int adj, std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_1c_and(unsigned mask, std::string prefix, std::string suffix = "");
dump_arg_instr_func_t dump_2c(std::string prefix, std::string interfix, std::string suffix = "");
dump_arg_instr_func_t dump_2c_add(unsigned add, std::string prefix, std::string interfix, std::string suffix = "");

}  // namespace instr

class OpcodeTable : public DispatchTable {
  std::map<unsigned, const OpcodeInstr*> instructions;
  std::vector<std::pair<unsigned, const OpcodeInstr*>> instruction_list;
  std::string name;
  Codepage codepage;
  bool final;

 public:
  OpcodeTable(std::string _name, Codepage cp) : name(_name), codepage(cp), final(false) {
  }
  OpcodeTable(const OpcodeTable&) = delete;
  OpcodeTable(OpcodeTable&&) = delete;
  OpcodeTable& operator=(const OpcodeTable&) = delete;
  OpcodeTable& operator=(OpcodeTable&&) = delete;
  ~OpcodeTable() override = default;
  DispatchTable* finalize() override;
  bool is_final() const override {
    return final;
  }
  int dispatch(VmState* st, CellSlice& cs) const override;
  std::string dump_instr(CellSlice& cs) const override;
  int instr_len(const CellSlice& cs) const override;
  bool insert_bool(const OpcodeInstr*);
  OpcodeTable& insert(const OpcodeInstr*);

 private:
  const OpcodeInstr* lookup_instr(unsigned opcode, unsigned bits) const;
  const OpcodeInstr* lookup_instr(const CellSlice& cs, unsigned& opcode, unsigned& bits) const;
};

class OpcodeInstrDummy : public OpcodeInstr {
 public:
  OpcodeInstrDummy() = delete;
  OpcodeInstrDummy(unsigned _minopc, unsigned _maxopc) : OpcodeInstr(_minopc, _maxopc) {
  }
  ~OpcodeInstrDummy() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
};

class OpcodeInstrSimple : public OpcodeInstr {
  unsigned char opc_bits;
  std::string name;
  exec_instr_func_t exec_instr;

 public:
  OpcodeInstrSimple() = delete;
  OpcodeInstrSimple(unsigned opcode, unsigned _opc_bits, std::string _name, exec_instr_func_t exec);
  ~OpcodeInstrSimple() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
  std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const override;
  int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const override;
};

class OpcodeInstrSimplest : public OpcodeInstr {
  unsigned char opc_bits;
  std::string name;
  exec_simple_instr_func_t exec_instr;

 public:
  OpcodeInstrSimplest() = delete;
  OpcodeInstrSimplest(unsigned opcode, unsigned _opc_bits, std::string _name, exec_simple_instr_func_t exec);
  ~OpcodeInstrSimplest() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
  std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const override;
  int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const override;
};

class OpcodeInstrFixed : public OpcodeInstr {
  unsigned char opc_bits, tot_bits;
  std::string name;
  dump_arg_instr_func_t dump_instr;
  exec_arg_instr_func_t exec_instr;

 public:
  OpcodeInstrFixed() = delete;
  OpcodeInstrFixed(unsigned opcode, unsigned _opc_bits, unsigned _arg_bits, dump_arg_instr_func_t dump,
                   exec_arg_instr_func_t exec);
  OpcodeInstrFixed(unsigned opcode_min, unsigned opcode_max, unsigned _tot_bits, unsigned _arg_bits,
                   dump_arg_instr_func_t dump, exec_arg_instr_func_t exec);
  ~OpcodeInstrFixed() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
  std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const override;
  int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const override;
};

class OpcodeInstrExt : public OpcodeInstr {
  unsigned char opc_bits, tot_bits;
  dump_instr_func_t dump_instr;
  exec_instr_func_t exec_instr;
  compute_instr_len_func_t compute_instr_len;

 public:
  OpcodeInstrExt() = delete;
  OpcodeInstrExt(unsigned opcode, unsigned _opc_bits, unsigned _arg_bits, dump_instr_func_t dump,
                 exec_instr_func_t exec, compute_instr_len_func_t comp_len);
  OpcodeInstrExt(unsigned opcode_min, unsigned opcode_max, unsigned _tot_bits, unsigned _arg_bits,
                 dump_instr_func_t dump, exec_instr_func_t exec, compute_instr_len_func_t comp_len);
  ~OpcodeInstrExt() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
  std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const override;
  int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const override;
};

class OpcodeInstrWithVersion : public OpcodeInstr {
 public:
  OpcodeInstrWithVersion() = delete;
  OpcodeInstrWithVersion(OpcodeInstr* instr, int required_version) :
      OpcodeInstr(instr->get_opcode_min(), instr->get_opcode_max()), instr(instr), required_version(required_version) {
  }
  ~OpcodeInstrWithVersion() override = default;
  int dispatch(VmState* st, CellSlice& cs, unsigned opcode, unsigned bits) const override;
  std::string dump(CellSlice& cs, unsigned opcode, unsigned bits) const override;
  int instr_len(const CellSlice& cs, unsigned opcode, unsigned bits) const override;
 private:
  OpcodeInstr* instr;
  int required_version;
};

}  // namespace vm
