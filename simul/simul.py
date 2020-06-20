import re
import os
import sys
from pathlib import Path

import networkx as nx

source_folder = sys.argv[1]
license_file = sys.argv[2]


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
    return f.readlines()


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
    ft_dict[get_include_token(source_file)] = read_file(source_file)
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
  warning_text = \
"""/* 
  THIS SINGLE HEADER FILE WAS AUTO-GENERATED USING `simul/simul.py`.
  DO NOT MAKE CHANGES HERE.
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

  with open(sys.argv[3], "w") as f:
    f.write(final_code)
