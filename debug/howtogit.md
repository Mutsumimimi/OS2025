1. 下载git
2. 在终端中编辑 	用户名 和 邮箱地址
  `$ git config --global user.name "John Doe"`
  `$ git config --global user.email johndoe@example.com`

  - 查看配置

    `$ git config --list `
3. 创建仓库
   `$ git init` 

   - 用 `git status` 来查看当前 git 状态（ unstaged, staged, commit ）

4. 暂存更改

   `$ git add ./repo1/readme.md`

   `$ git add ./repo2/`

5. 提交（ commit ）

   - 这里的提交仍是本地的操作，不涉及远程仓库

   `$ git commit`

6. 远程仓库

   - 方案一：

     1. 先手动打开github.com，手动添加一个仓库`myrepo`

     2. 在终端使用如下命令：

        `$ git remote add origin git@github.com:username/myrepo.git `

        - ps: 删除关联的操作：`git remote remove origin`

     3. 推送本地内容到远程库：
        - 初次推送：`git push -u origin master`
        - 后面推送就简单了：`git push`

   - 方案二：

     1. 直接在终端使用如下命令：

        ` $ git remote add origin https://github.com/username/myrepo.git`

        或在vscode里操作，按绿色按钮[发布Branch].

     2. 后续操作同方案一  

Ref: 

​	`https://vlab.ustc.edu.cn/docs/tutorial/git/` USTC-Linux101

​	`https://git-scm.com/book/en/v2` [Git 官网教程]

