```
# FIXME: Specify when newlines are allowed
# FIXME: Arrays

module
    statements

statements
    statement
    statement nl statements

statement
    comment
    use_statement
    variable_definition
    function_definition
    type_definition
    variable_assignment
    function_call
    method_call
    if_statement
    while_statement
    return_statement
    assertion

comment
    '#' * nl

use_statement
    "use" ws module_name

module_name
    name

name
    name_start_char
    name_start_char name_chars

name_start_char
    'a' . 'z'
    'A' . 'Z'
    '_'

name_chars
    name_char
    name_char name_char

name_char
    name_start_char
    digit

variable_defintion
    type_name ws variable_name
    type_name ws variable_name ws '=' ws expression

type_name
    name

expression
    constant
    variable_name
    variable_member
    function_call
    method_call
    unary_operation
    binary_operation

constant
    boolean_constant
    integer_constant
    hex_constant
    string_constant_single
    string_constant_double

boolean_constant
    "true"
    "false"

integer_constant
    digit
    onenine digits
    '-' digit
    '-' onenine digits

digits
    digit
    digit digits

digit
    '0'
    onenine

onenine
    '1' . '9'

bin_constant
    "0b" bin_digits

bin_digits
    bin_digit
    bin_digit bin_digits

bin_digit
    '0'
    '1'

hex_constant
    "0x" hex_digits

hex_digits
    hex_digit
    hex_digit hex_digits

hex_digit
    digit
    'A' . 'F'
    'a' . 'f'

string_constant_single
    "'" characters_single "'"

characters_single
    ""
    character_single characters_single

character_single
    '0020' . '10FFFF' - '"' - '\'
    '\' escape

string_constant_double
    '"' characters_double '"'

characters_double
    ""
    character_double characters_double

character_double
    '0020' . '10FFFF' - "'" - '\'
    '\' escape

escape
    '"'
    "'"
    '\'
    'n'
    'r'
    't'
    'x' hex_digit hex_digit
    'u' hex_digit hex_digit hex_digit hex_digit
    'U' hex_digit hex_digit hex_digit hex_digit hex_digit hex_digit hex_digit hex_digit

variable_name
    name

variable_member
    expression '.' member_name

member_name
    name

function_call
    function_name ws '(' arguments ')'

function_name
    name

method_call
    expression '.' member_name ws '(' arguments ')'

unary_operation
    '-' expression

binary_operation
    expression '+' expression
    expression '-' expression
    expression '/' expression
    expression '*' expression
    expression "and" expression
    expression "or" expression
    expression "xor" expression

function_defintion
    type_name ws function_name ws statement_block

statement_block
    '{' ws nl statements '}'

type_defintion
    "type" ws type_name ws '{' type_statements '}'

type_statements
    type_statement
    type_statement nl type_statements

type_statement
    variable_definition # Note, assignment not supported
    function_definition

variable_assignment
    variable ws '=' ws expression
    variable_member ws '=' ws expression

function_call
    function_name ws '(' arguments ')'

arguments
    argument
    argument ',' arguments

if_statement
    "if" condition ws statement_block
    "if" condition ws statement_block "else" statement_block

while_statement
    "while" condition ws statement_block

condition
    expression
    expression '==' expression
    expression '!=' expression
    expression '>' expression
    expression '>=' expression
    expression '<' expression
    expression '<=' expression

return_statement
    "return" ws expression

assertion
    "assert" expression

nl
   '000A'

ws
    ""
    '0020' ws
    '000D' ws
    '0009' ws
```
