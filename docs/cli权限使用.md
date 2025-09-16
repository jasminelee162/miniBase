按下面步骤操作：
1) 先用默认管理员登录
- 在提示时输入：
  - 用户名: root
  - 密码: root

2) 登录成功后创建新用户
- 在主循环里输入：
```
.users
.create_user qingchen 123456 developer   # 角色可选：dba / developer / analyst（小写）
.list_users
.back
```

3) 切换登录测试新用户
```
.logout
.login
用户名: qingchen
密码: 123456
.info
```

4) 验证权限
- developer：可 CREATE_TABLE/SELECT/INSERT/UPDATE/DELETE
- analyst：只有 SELECT（例如 INSERT 会被拒绝）
- dba：拥有全部权限，可管理用户