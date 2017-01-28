#!/usr/bin/env python3
# https://gist.githubusercontent.com/pib/240957/raw/093bc0b1e6d5188af7b5df674a5af37d4927d5f0/sexp.py
#
# Copyright (c) 2009 Paul Bonser

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


import ast
import sys
import unittest

from string import whitespace

atom_end = set('()"\'') | set(whitespace)

def close_last(item):
    if (type(item[-1]) == tuple):
        if len(item[-1]) == 1:
            item[-1] = item[-1][0]
    elif (type(item[-1]) == list):
        close_last(item[-1])

def _parse(sexp):
    stack, i, length = [[]], 0, len(sexp)
    while i < length:
        c = sexp[i]
        cn = sexp[i+1] if (i+1 < length) else None
        escaped = False
        if (c == '\\' and cn == '"'):
            c = cn
            i += 1
            escaped = True

        reading = type(stack[-1])
        #print(c, stack, reading, escaped)
        if reading == list:
            if c == '(':
                stack.append([])
            elif c == ')':
                stack[-2].append(stack.pop())
                if stack[-1][0] == ('quote',):
                    stack[-2].append(stack.pop())
            elif c == '"':
                stack.append('')
            elif c == "'":
                stack.append([('quote',)])
            elif c in whitespace:
                close_last(stack[-1])
            else:
                stack.append((c,))
        elif reading == type(''):
            if c == '"' and not escaped:
                stack[-2].append(stack.pop())
                if stack[-1][0] == ('quote',):
                    stack[-2].append(stack.pop())
            elif c == '\\':
                i += 1
                stack[-1] += sexp[i]
            else:
                stack[-1] += c
        elif reading == tuple:
            if c in atom_end:
                atom = stack.pop()
                if atom[0][0].isdigit():
                    stack[-1].append(ast.literal_eval(atom[0]))
                else:
                    stack[-1].append(atom)
                if stack[-1][0] == ('quote',):
                    stack[-2].append(stack.pop())
                close_last(stack[-1])
                continue
            else:
                stack[-1] = ((stack[-1][0] + c),)
        i += 1
    return stack.pop()

def _thread_last(sexp):
    """Handles thread last macro similar to clojure.

       Expansion proceeds from right to left unlike
       clojure, which expands macros left to right.
    """
    ret = sexp
    if sexp[0] == "->>" and len(sexp) > 2:
        ret = sexp[1]
        for e in sexp[2:]:
            if not isinstance(e, list):
                e = [e]
            else:
                e = _thread_last(e)
            e.append(ret)
            ret = e
    return ret

def parse(sexp):
    s = _parse(sexp)[0]
    return _thread_last(s)

class Tests(unittest.TestCase):

    def test_simple(self):
        self.assertEqual(parse("(a (b c))"), ['a', ['b', 'c']])

    def test_escape(self):
        self.assertEqual(parse("(a (\"b foo\" c))"), ['a', ['b foo', 'c']])
        self.assertEqual(parse("(a (\"b \\\" foo\" c))"), ['a', ['b " foo', 'c']])

    def test_quote(self):
        #self.assertEqual(parse("(a (b 'c))"), ['a', ['b', ['quote', 'c']]])
        pass

    def test_thread_last(self):
        self.assertEqual(parse("(->> a b)"), ['b', 'a'])
        self.assertEqual(parse("(->> a b c)"), ['c', ['b', 'a']])
        self.assertEqual(parse("(->> (a) (b) (c))"), ['c', ['b', ['a']]])
        self.assertEqual(parse("(->> (a) (b c) (d))"), ['d', ['b', 'c', ['a']]])
        self.assertEqual(parse("(->> a (b) (c) (->> d e f))"),
                         ['f', ['e', 'd'], ['c', ['b', 'a']]])

if __name__ == '__main__':
    if len(sys.argv) == 1:
        unittest.main()
    print(parse(sys.argv[1]))
