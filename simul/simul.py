import re
import os
import sys

import datetime
from pathlib import Path

import networkx as nx

source_folder = "include/shadesmar"
license_file = "LICENSE"


def comment_remover(text):
  def replacer(match):
    s = match.group(0)
    if s.startswith('/'):
      return " "  # note: a space and not an empty string
    else:
      return s

  pattern = re.compile(
      r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
      re.DOTALL | re.MULTILINE)
  return re.sub(pattern, replacer, text)


def fuse_lines(text_lines):
  text_lines = filter(lambda line: len(line.strip()) != 0, text_lines)
  return ''.join(text_lines)


def read_file(filename):
  text_lines = []
  with open(filename) as f:
    text_lines = f.readlines()

  # Add newline at end of file
  if len(text_lines) > 0:
    text_lines[-1] += "\n"
  return text_lines


def remove_guards(code):
  # [str]
  guard_stack = []
  guard_names = []
  del_lines = []
  for lno, line in enumerate(code):
    if line.startswith("#if"):
      guard_stack.append(lno)
    if line.startswith("#endif"):
      if_line = code[guard_stack[-1]]
      if "#ifndef INCLUDE" in if_line:
        del_lines.append(guard_stack[-1])
        del_lines.append(lno)
        guard_names.append(if_line[len("#ifndef "):-1])
      guard_stack.pop()

  for lno, line in enumerate(code):
    for gname in guard_names:
      if line.startswith("#define " + gname):
        del_lines.append(lno)

  for index in sorted(del_lines, reverse=True):
    if "INCLUDE" in code[index]:
      del code[index]

  return code


def crawl_src_folder(folder):
  return [path for path in Path(folder).rglob('*.h')]


def get_include_token(full_path):
  return str(Path(*full_path.parts[1:]))


def init_graph(source_files):
  G = nx.DiGraph()
  for source_file in source_files:
    G.add_node(get_include_token(source_file))
  return G


def get_filename_text_dict(source_files):
  ft_dict = {}
  for source_file in source_files:
    code_with_guards = read_file(source_file)
    ft_dict[get_include_token(source_file)] = remove_guards(code_with_guards)
  return ft_dict


def process_includes(source_code):
  shm_inc = []
  other_inc = []
  good_source_code = []
  for line in source_code:
    if "#include" in line:
      mod = line[len("#include"):].strip()[1:-1]
      if "shadesmar" in mod:
        shm_inc.append(mod)
        continue
      else:
        other_inc.append(mod)
    good_source_code.append(line)

  return shm_inc, other_inc, good_source_code


def add_deps_and_get_incs(g, ft_dict):
  global_inc = []
  for filename, code in ft_dict.items():
    shm_inc, other_inc, good_source_code = process_includes(code)
    global_inc += other_inc

    for s in shm_inc:
      g.add_edge(s, filename)

    ft_dict[filename] = good_source_code

  return sorted(list(set(global_inc)))


def topo_sort(g):
  return list(nx.topological_sort(g))


def get_single_header(order, ft_dict, incs):
  header_lines = []

  for o in order:
    header_lines += ft_dict[o]

  include_head = ["#ifndef SHADESMAR_SINGLE_H_\n#define SHADESMAR_SINGLE_H_\n"]
  include_end = ["#endif\n"]
  return include_head + header_lines + include_end


def make_pretty(code):
  code = comment_remover(fuse_lines(code))
  lines = code.split('\n')
  lines = list(filter(lambda l: l.strip() != '', lines))
  code = '\n'.join(lines)
  return code


def finalize_code(code):
  dt = str(datetime.datetime.utcnow())
  warning_text = \
"""/* 
  THIS SINGLE HEADER FILE WAS AUTO-GENERATED USING `simul/simul.py`.
  DO NOT MAKE CHANGES HERE.

  GENERATION TIME: """ + dt + """ UTC
*/\n\n"""
  license_text = "/*" + ''.join(read_file(license_file)) + "*/\n\n"

  return license_text + warning_text + code


# TODO: Remove per-file header guards
if __name__ == '__main__':
  src_paths = crawl_src_folder(source_folder)
  ft_dict = get_filename_text_dict(src_paths)
  g = init_graph(src_paths)
  inc = add_deps_and_get_incs(g, ft_dict)
  order = topo_sort(g)
  pure_code = get_single_header(order, ft_dict, inc)
  clean_code = make_pretty(pure_code)
  final_code = finalize_code(clean_code)

  with open("include/shadesmar.h", "w") as f:
    f.write(final_code)
