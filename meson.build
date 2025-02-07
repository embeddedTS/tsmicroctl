project('tsmicroctl', 'c',
  default_options: ['c_args=-Wall'])
gpiod_dep = dependency('libgpiod')

executable('tsmicroctl', 
  [
    'tsmicroctl.c',
    'micro.c',
    'ts7100.c',
    'ts7180.c',
    'ts7800v2.c',
  ], 
  dependencies : [gpiod_dep],
  install : true
)
