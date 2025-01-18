#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./dcc "$input" > tmp.s || exit
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 2 2
assert 100 100
assert 6 '4+5-3'
assert 41 '12 + 34 - 5'
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'

echo OK

