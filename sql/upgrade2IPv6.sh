mysql -uroot -proot@appinside db_tars -e "alter table t_config_files modify column host varchar(50) not null default '';"
mysql -uroot -proot@appinside db_tars -e "alter table t_node_info modify column endpoint_ip varchar(50) default '';"
mysql -uroot -proot@appinside db_tars -e "alter table t_task_item modify column node_name varchar(50) default null;"

