project ('elf', [ 'c' ],
         version: '0.1',
         default_options : [ 'c_std=c11' ])

version = run_command ('make-version')
elf = executable ('elf',
                  [ 'elf.c',
                    'elf-operation.c',
                    'elf-parser.c',
                    'elf-runner.c',
                    'elf-token.c',
                    'utils.c',
                    'x86_64.c',
                  ],
                  c_args: [ '-DVERSION="@0@"'.format (version.stdout ().strip ()) ],
                  install: true)

test_runner = executable ('test-runner',
                          [ 'test-runner.c',
                            'utils.c',
                          ])

foreach test : [ 'empty-file',
                 'hello-world',
                 'add',
                 'subtract',
                 'multiply',
               ]
  test (test, test_runner, args : [ elf.full_path (), '@0@/tests/@1@.elf'.format (meson.current_source_dir (), test) ])
endforeach
