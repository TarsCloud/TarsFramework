
#all salve install

#tarsqueryproperty
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsqueryproperty','localip.tars.com','tars.tarsqueryproperty.NoTarsObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18693',200000,'','tars.tarsqueryproperty.NoTarsObj',18693,60000,now(),NULL,'not_tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsqueryproperty','','localip.tars.com',now(),'','/usr/local/app/tars/tarsqueryproperty/bin/tarsqueryproperty','tars.tarsproperty',0,'active','inactive',0,'1.0.1',now(),'','1.0.1',now(),NULL,'tars_cpp','');

#tarsconfig
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsconfig','localip.tars.com','tars.tarsconfig.ConfigObjAdapter',now(),10,'tcp -h localip.tars.com -t 60000 -p 18193',200000,'','tars.tarsconfig.ConfigObj',18193,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsconfig','','localip.tars.com',now(),'','/usr/local/app/tars/tarsconfig/bin/tarsconfig','tars.tarsconfig',0,'active','inactive',0,'1.0.1',now(),'','1.0.1','2015-08-08 10:49:37','admin','tars_cpp');

#tarsnotify
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsnotify','localip.tars.com','tars.tarsnotify.NotifyObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18593',200000,'','tars.tarsnotify.NotifyObj',18593,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsnotify','','localip.tars.com',now(),'','/usr/local/app/tars/tarsnotify/bin/tarsnotify','tars.tarsnotify',0,'active','inactive',0,'1.0.1',now(),'','1.0.1',now(),'admin','tars_cpp');

#tarsproperty
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsproperty','localip.tars.com','tars.tarsproperty.PropertyObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18493',200000,'','tars.tarsproperty.PropertyObj',18493,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsproperty','','localip.tars.com',now(),'','/usr/local/app/tars/tarsproperty/bin/tarsproperty','tars.tarsproperty',0,'active','inactive',0,'1.0.1',now(),'','1.0.1',now(),'admin','tars_cpp');

#tarsquerystat
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsquerystat','localip.tars.com','tars.tarsquerystat.NoTarsObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18393',200000,'','tars.tarsquerystat.NoTarsObj',18393,60000,now(),'admin','not_tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`, `profile`) VALUES ('tars','tarsquerystat','','localip.tars.com',now(),'','/usr/local/app/tars/tarsquerystat/bin/tarsquerystat','tars.tarsquerystat',0,'active','inactive',0,'1.0.1',now(),'','1.0.1',now(),'admin','tars_cpp','');

#tarsstat
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarsstat','localip.tars.com','tars.tarsstat.StatObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18293',200000,'','tars.tarsstat.StatObj',18293,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarsstat','','localip.tars.com',now(),'','/usr/local/app/tars/tarsstat/bin/tarsstat','tars.tarsstat',0,'active','inactive',0,'1.0.1',now(),'','1.1.0',now(),'admin','tars_cpp');

