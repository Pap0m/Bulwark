src_files = files(
    'main.c',
    'db.c'
)

executable('Bulwark',
    src_files,
    dependencies: [
        mongoose_dep,
        sqlite3_dep
    ],
    install: false
)
