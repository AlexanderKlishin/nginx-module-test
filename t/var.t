# vi:filetype=

use lib 'lib';
use Test::Nginx::Socket;

plan tests => 2 * blocks();

#$Test::Nginx::LWP::LogLevel = 'debug';

run_tests();

__DATA__

=== TEST 1: simple
--- config
    location /testvar {
        set $var "value";
        print_var "$var";
    }
--- request
    GET /testvar
--- response_body eval
"value"

=== TEST 2: complex
--- config
    location /testvar {
        set $var "value";
        print_var "--${var}--";
    }
--- request
    GET /testvar
--- response_body eval
"--value--"

=== TEST 3: static
--- config
    location /testvar {
        print_var "static";
    }
--- request
    GET /testvar
--- response_body eval
"static"
