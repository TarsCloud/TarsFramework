#only master install
#tarslog
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarslog','localip.tars.com','tars.tarslog.LogObjAdapter',now(),5,'tcp -h localip.tars.com -t 60000 -p 18993',200000,'','tars.tarslog.LogObj',18893,60000,now(),NULL,'tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarslog','','localip.tars.com',now(),'','/usr/local/app/tars/tarslog/bin/tarslog','tars.tarslog',0,'active','active',0,'1.0.1',now(),'','1.0.1',now(),'admin','tars_cpp');

#tarspatch
replace into `t_adapter_conf` (`application`, `server_name`, `node_name`, `adapter_name`,`registry_timestamp`, `thread_num`, `endpoint`, `max_connections`, `allow_ip`, `servant`, `queuecap`, `queuetimeout`,`posttime`,`lastuser`,`protocol`, `handlegroup`) VALUES ('tars','tarspatch','localip.tars.com','tars.tarspatch.PatchObjAdapter',now(),10,'tcp -h localip.tars.com -t 60000 -p 18793',200000,'','tars.tarspatch.PatchObj',18793,60000,now(),'admin','tars','');

replace into `t_server_conf` (`application`, `server_name`, `node_group`, `node_name`, `registry_timestamp`, `base_path`, `exe_path`, `template_name`, `bak_flag`, `setting_state`, `present_state`, `process_id`, `patch_version`, `patch_time`, `patch_user`, `tars_version`, `posttime`, `lastuser`, `server_type`) VALUES ('tars','tarspatch','','localip.tars.com',now(),'','/usr/local/app/tars/tarspatch/bin/tarspatch','tars.tarspatch',0,'active','active',0,'1',now(),'','1.0.1',now(),'admin','tars_cpp');


