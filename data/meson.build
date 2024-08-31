desktop_file = app_id + '.desktop'
install_data(desktop_file,
             install_dir : join_paths(get_option('prefix'), 'share', 'applications'))

appstream_file = app_id + '.metainfo.xml'
install_data(appstream_file,
             install_dir : join_paths(get_option('prefix'), 'share', 'metainfo'))

icon_file = app_id + '.png'
icon_dir = join_paths(get_option('prefix'), 'share', 'icons', 'hicolor', '512x512', 'apps')
install_emptydir(icon_dir)
install_data(icon_file,
             install_dir : icon_dir)

meson.add_install_script('install_config.sh')
meson.add_install_script('install_data_dir.sh')