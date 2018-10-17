mysql -uroot -proot@appinside -e "create database db_tars"
mysql -uroot -proot@appinside -e "create database tars_stat"
mysql -uroot -proot@appinside -e "create database tars_property"
mysql -uroot -proot@appinside -e "create database db_tars_web"
mysql -uroot -proot@appinside db_tars < db_tars.sql
mysql -uroot -proot@appinside db_tas_web < db_tars_web.sql
