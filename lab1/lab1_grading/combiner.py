#!/usr/bin/env python3

import os, sys, re, getopt, csv

BASEPATH = os.getcwd()
OUTPUT_F = os.path.join(BASEPATH, 'results.csv')
ERROR_F = os.path.join(BASEPATH, 'grader.err')
COMBINE_F = os.path.join(BASEPATH, 'results_err.csv')

def usage(rc=2):
  print('Usage:')
  print('  %s' % sys.argv[0] + '\t  [-h | --help]' + \
          '\n\t\t  [-o <outputfile (%s/results.csv)>]' % BASEPATH + \
          '\n\t\t  [-e <errorfile (%s/grader.err)>]' % BASEPATH + \
          '\n\t\t  [-s <combinedfile (%s/results_err.csv)>]' % BASEPATH
          )
  print()
  sys.exit(rc)

def get_key(_id, _ex_no):
  return tuple([_id.upper(), _ex_no.upper()])


def main():
  global OUTPUT_F, ERROR_F, COMBINE_F

  try:
    opts, args = getopt.getopt(sys.argv[1:],"ho:e:s:",["help", "outputfile=", "errorfile=", "combinedfile="])
  except getopt.GetoptError as e:
    print(e)
    usage(1)
  for o, a in opts:
    if o == '-h' or o == '--help':
      usage(0)
    if o == '-o' or o == '--outputfile':
      OUTPUT_F = os.path.abspath(a)
    if o == '-e' or o == '--errorfile':
      ERROR_F = os.path.abspath(a)
    if o == '-s' or o == '--combinedfile':
      COMBINE_F = os.path.abspath(a)

  if not all(os.path.isfile(filename) for filename in [OUTPUT_F, ERROR_F]):
    print('Input file(s) not found. Exiting')

  fields = []
  with open(OUTPUT_F, 'r') as csv_file:
    csv_dict_reader = csv.DictReader(csv_file)
    column_names = csv_dict_reader.fieldnames
    fields.extend(column_names)
  fields.extend([ '_'.join([ex_no, 'Errors']) for ex_no in fields if ex_no.startswith('Ex') ])

  id_ex_comment = {}
  with open(ERROR_F, 'r') as errf:
    for line in errf:
      res  = re.findall(r"\[(\w+)\]", line)
      _id = res[0]
      _ex_no = res[1]
      _comment_list = list(filter(None, re.findall(r"(.*?)(?:\[.*?\]|$)", line)))
      _comment = ';'.join([el.replace(',', '-').replace(';', '-').strip() for el in _comment_list])

      if get_key(_id, _ex_no) in id_ex_comment:
        id_ex_comment[get_key(_id, _ex_no)].append(_comment)
      else:
        id_ex_comment[get_key(_id, _ex_no)] = [_comment]


  with open(OUTPUT_F, 'r') as csv_file_reader, open(COMBINE_F, 'w') as csv_file_writer :
    csv_dict_reader = csv.DictReader(csv_file_reader)
    csv_dict_writer = csv.DictWriter(csv_file_writer, fieldnames=fields)
    csv_dict_writer.writeheader()

    for row in csv_dict_reader:
      row_out = {}
      for f in fields:
        row_out[f] = (row[f] if 'Errors' not in f else ( ';'.join(id_ex_comment[get_key(row['StudentID'], f.split('_')[0])])  if get_key(row['StudentID'], f.split('_')[0]) in id_ex_comment else 'None'))
      csv_dict_writer.writerow(row_out)

if __name__ == '__main__':
  main()
