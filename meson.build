project('Bulwark', 'c',
    version: '0.1',
    default_options: ['warning_level=3']
)

sqlite3_dep = dependency('sqlite3', fallback: ['sqlite3', 'sqlite3_dep'], required: true)

mongoose_inc = include_directories('c_libs/mongoose')
mongoose_lib = static_library('mongoose',
    'c_libs/mongoose/mongoose.c',
    include_directories: mongoose_inc
)
mongoose_dep = declare_dependency(
    link_with: mongoose_lib,
    include_directories: mongoose_inc
)

src_dir = include_directories('src')

subdir('src')
