# 数据库设置

服务端连接 MySQL 中的 `WeChat` 数据库。以下建表语句按当前账户查询和注册代码整理，供空白本地数据库初始化：

```sql
CREATE DATABASE IF NOT EXISTS WeChat
    DEFAULT CHARACTER SET utf8
    DEFAULT COLLATE utf8_general_ci;

USE WeChat;

CREATE TABLE IF NOT EXISTS t_user (
    id INT NOT NULL AUTO_INCREMENT,
    tel VARCHAR(40) NOT NULL,
    password VARCHAR(40) NOT NULL,
    name VARCHAR(40) NOT NULL,
    icon INT NOT NULL DEFAULT 1,
    feeling VARCHAR(40) NOT NULL DEFAULT '',
    PRIMARY KEY (id),
    UNIQUE KEY uq_user_tel (tel),
    UNIQUE KEY uq_user_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

可将上述语句保存到本机临时文件后导入，也可在 MySQL 客户端中直接执行；该 schema 不应视为经过数据库集成测试的迁移脚本。

应用只需读取账户并在注册时插入账户。可为服务端创建专用账号，并限制其在用户表中的权限：

```sql
CREATE USER 'meeting_app'@'localhost' IDENTIFIED BY 'replace-with-a-local-password';
GRANT SELECT, INSERT ON WeChat.t_user TO 'meeting_app'@'localhost';
```

当前版本的数据库连接参数硬编码在 `Server/include/packdef.h`。运行前需根据本地 MySQL 主机、账号、口令和数据库修改这些值。不要把真实口令提交到仓库；数据库配置改成环境变量或本地配置文件应作为后续代码改进。

服务端使用的 `t_user` 字段包括 `id`、`tel`、`password`、`name`、`icon` 和 `feeling`。客户端注册时写入手机号、口令摘要和用户名，其余字段由数据库默认值补充。该 schema 适用于本地演示；迁移到现有数据库前应先核对表结构和数据。
