
-- all salve install

-- tarsregistry
replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsregistry','','localip.tars.com',now(),'','/usr/local/app/tars/tarsregistry/bin/tarsregistry','tars.tarsregistry',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp','');
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsregistry','localip.tars.com','tars.tarsregistry.QueryObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 17890',100000,'','tars.tarsregistry.QueryObj',100000,10000,now(),'admin','tars','');
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsregistry','localip.tars.com','tars.tarsregistry.RegistryObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 17891',100000,'','tars.tarsregistry.RegistryObj',100000,10000,now(),'admin','tars','');

-- tarsAdminRegistry
#replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsAdminRegistry','localip.tars.com','tars.tarsAdminRegistry.AdminRegObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 12000',2000,'','tars.tarsAdminRegistry.AdminRegObj',100000,60000,now(),'admin','tars','');
#replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsAdminRegistry','','localip.tars.com',now(),'','/usr/local/app/tars/tarsAdminRegistry/bin/tarsAdminRegistry','tars.tarsAdminRegistry',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp','');

-- tarsqueryproperty
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsqueryproperty','localip.tars.com','tars.tarsqueryproperty.QueryObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18693',200000,'','tars.tarsqueryproperty.QueryObj',100000,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsqueryproperty','','localip.tars.com',now(),'','/usr/local/app/tars/tarsqueryproperty/bin/tarsqueryproperty','tars.tarsqueryproperty',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp','');

-- tarsconfig
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsconfig','localip.tars.com','tars.tarsconfig.ConfigObjAdapter',now(),10,'tcp -h localip.tars.com -t 60000 -p 18193',200000,'','tars.tarsconfig.ConfigObj',100000,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsconfig','','localip.tars.com',now(),'','/usr/local/app/tars/tarsconfig/bin/tarsconfig','tars.tarsconfig',0,'active','active',0,'2.1.0',now(),'','2.1.0','2015-08-08 10:49:37','admin','tars_cpp');

-- tarsnotify
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsnotify','localip.tars.com','tars.tarsnotify.NotifyObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18593',200000,'','tars.tarsnotify.NotifyObj',100000,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsnotify','','localip.tars.com',now(),'','/usr/local/app/tars/tarsnotify/bin/tarsnotify','tars.tarsnotify',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp');

-- tarsquerystat
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsquerystat','localip.tars.com','tars.tarsquerystat.QueryObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18393',200000,'','tars.tarsquerystat.QueryObj',100000,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsquerystat','','localip.tars.com',now(),'','/usr/local/app/tars/tarsquerystat/bin/tarsquerystat','tars.tarsquerystat',0,'active','active',0,'2.1.0',now(),'','2.1.0',now(),'admin','tars_cpp','');


-- v2.1.0, tarsquerystat & tarsqueryproperty change to tars server
delete from t_adapter_conf where application = 'tars' and server_name = 'tarsqueryproperty' and adapter_name = 'tars.tarsqueryproperty.NoTarsObjAdapter';
delete from t_adapter_conf where application = 'tars' and server_name = 'tarsquerystat' and adapter_name = 'tars.tarsquerystat.NoTarsObjAdapter';
