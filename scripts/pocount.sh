#!/bin/sh

number_of_strings=`egrep -c '^\<msgid\>.' $1`
number_of_untranslated_strings=`egrep -c '^\<msgstr\>+ \"\"' $1`
number_of_invalid_empties=`egrep -c '^\<msgid\>+ \"\"' $1`

number_of_untranslated_strings=`expr $number_of_untranslated_strings - $number_of_invalid_empties`

echo $2 `expr 100 - $number_of_untranslated_strings \* 100 / $number_of_strings`
