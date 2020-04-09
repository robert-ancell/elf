project ('elf', [ 'c' ],
         version: '0.1',
         default_options : [ 'c_std=c11' ])

executable ('elf',
             [ 'elf.c',
               'elf-operation.c',
               'elf-parser.c',
               'elf-runner.c',
               'elf-token.c',
               'utils.c',
             ],
             install: true)
