metainfo_dir = join_paths(get_option('prefix'), 'share', 'metainfo')
install_data('io.github.b43NnUNF4vidFYFhpqaLWy2ANawtRbMtUXZY9Pf.jumpdf.metainfo.xml', install_dir : metainfo_dir)

install_data(app_id + '.desktop',
             install_dir : join_paths(get_option('prefix'), 'share', 'applications'))

icon_dir = join_paths(get_option('prefix'), 'share', 'icons', 'hicolor', '512x512', 'apps')
install_emptydir(icon_dir)
install_data('io.github.b43NnUNF4vidFYFhpqaLWy2ANawtRbMtUXZY9Pf.jumpdf.png', install_dir : icon_dir)

meson.add_install_script('install_config.sh')
meson.add_install_script('install_data_dir.sh')