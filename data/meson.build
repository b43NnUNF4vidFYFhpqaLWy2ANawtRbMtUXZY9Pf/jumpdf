conf = configuration_data()
conf.set('app_name', app_name)
conf.set('app_id', app_id)
conf.set('app_version', version)
conf.set('developer_id', developer_id)

desktop_conf = configure_file(
    input: 'app.desktop.in',
    output: '@0@.desktop.in'.format(app_id),
    configuration: conf
)

desktop_file = i18n.merge_file(
    input: desktop_conf,
    output: '@0@.desktop'.format(app_id),
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: get_option('datadir') / 'applications'
)

appstream_conf = configure_file(
    input: 'app.metainfo.xml.in',
    output: '@0@.metainfo.xml.in'.format(app_id),
    configuration: conf
)

appstream_file = i18n.merge_file(
    input: appstream_conf,
    output: '@0@.metainfo.xml'.format(app_id),
    po_dir: '../po',
    install: true,
    install_dir: get_option('datadir') / 'metainfo'
)

icon_conf = configure_file(
    input: 'app.png',
    output: '@0@.png'.format(app_id),
    copy: true
)

install_data(icon_conf,
             install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '512x512', 'apps'))

meson.add_install_script('install_config.sh')
meson.add_install_script('install_data_dir.sh')