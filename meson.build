project ('elf', [ 'c' ],
         version: '0.1',
         default_options : [ 'c_std=c11' ])

version = run_command ('make-version')
executable ('elf',
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
