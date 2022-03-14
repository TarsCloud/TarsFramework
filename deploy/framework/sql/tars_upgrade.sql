
-- upgrade sql
IF NOT EXISTS(SELECT 1 FROM COLUMNS WHERE TABLE_SCHEMA='db_tars' AND table_name='t_server_conf' AND COLUMN_NAME='run_type')
BEGIN
    ALTER TABLE `t_server_conf` ADD `run_type` VARCHAR(128) DEFAULT '';
END

IF NOT EXISTS(SELECT 1 FROM COLUMNS WHERE TABLE_SCHEMA='db_tars' AND table_name='t_server_conf' AND COLUMN_NAME='base_image_id') THEN
BEGIN
    ALTER TABLE `t_server_conf` ADD `base_image_id` INT(11) DEFAULT '0';
END