#!/usr/bin/env python
# Copyright 2018 Mobvoi Inc. All Rights Reserved.
# Author: spye@mobvoi.com (Shunping Ye)

import glob
import os
import subprocess
import sys

def should_filter(ignore_prefix, file_name):
    for prefix in ignore_prefix:
        if file_name.startswith(prefix) or (file_name in glob.glob(prefix)):
            return True
    return False


def get_changed_files():
    output = []
    result = []
    status_cmd = 'git status -uno |grep "modified:\\|new file:"'
    output = os.popen(status_cmd).read().split('\n')
    size = len(output)
    ignore_prefix = []

    with open("./code_style/.codestyleignore") as f:
        for line in f.readlines():
            ignore_prefix.append(line.strip())

    i = size - 2;
    while i > 0:
        # If we reach another empty line, exit.
        line = output[i]
        i -= 1
        if len(line) == 0:
            break

        # Skip invalid files
        parts = line.split(':')
        if len(parts) != 2:
            continue
        print(parts[1])
        if not should_filter(ignore_prefix, parts[1]):
            result.append(parts[1].strip())
    return result

def check_by_language(command, language, opened_files):
    # file list is empty , skip it.
    if len(opened_files) == 0:
        return 0;
    print('Begin to check %s code style for changed source code' % language)
    cmd = '%s %s' % (command, ' '.join(opened_files))
    p = subprocess.Popen(cmd, shell=True)
    try:
        p.wait()
        if p.returncode:
            if p.returncode == 127:
                msg = ("Can't execute '{0}' to check style, you can config the "
                       "'cpplint' path, make sure '{0}' command is correct.").format(command)
                print(msg)
            return -1
        else:
            return 0;
    except KeyboardInterrupt, e:
        print(str(e))
        return -1
    return 0

def check_code_style(cpp_files, java_files, python_files):
    if not cpp_files and not java_files and not python_files:
        return 0
    ret_code = 0
    # Check code style for c/cpp files.
    cpplint = 'python code_style/cpplint.py --filter=-build/include_what_you_use'
    if not cpplint:
        print('cpplint disabled')
    else:
        if check_by_language(cpplint, "cpp", cpp_files) != 0:
            ret_code = -1;

    # Check style for java files.
    # java_checkstyle = 'python code_style/javalint.py'
    # if not java_checkstyle:
    #    print('javalint disabled')
    # else:
    #    if check_by_language(java_checkstyle, "java", java_files) != 0:
    #        ret_code = -1;

    # Check sytle for python files
    # python_checkstyle = 'pylint -E --rcfile=code_style/googlecl-pylint.rc' 
    # if check_by_language(python_checkstyle, "python", python_files) != 0:
    #    ret_code = -1;

    return ret_code

if __name__ == '__main__':
    print "Begin to check code style:"
    files = get_changed_files()
    # call cpplint.py to check style.
    cpp_files = set()
    java_files = set()
    python_files = set()
    for f in files:
        fullpath = os.path.join(os.getcwd(), f)
        if f.endswith('.h') or f.endswith('.hpp') or f.endswith('.cc') or f.endswith('.cpp'):
            cpp_files.add(fullpath)
        elif f.endswith('.java'):
            java_files.add(fullpath)
        elif f.endswith('.py'):
            python_files.add(fullpath)

    if check_code_style(cpp_files, java_files, python_files) != 0:
        print "Please fixing style warnings before submitting the code!"
        sys.exit(-1)
    print "Done."
    sys.exit(0)
