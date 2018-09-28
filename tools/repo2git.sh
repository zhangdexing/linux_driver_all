read -p "请输入要提交的名字:" commit
find . -name ".git" -type d -exec rm -rf {} \;
find . -name ".gitignore" -type f -exec rm -rf {} \;
rm -rf .repo
git init
git add  -A
git commit -am "$commit"
