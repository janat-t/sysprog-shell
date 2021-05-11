sh -c 'echo $PPID  > .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt'
sh -c 'echo $PPID >> .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt' | sh -c 'cat ; echo $PPID >> .test.tmp.txt'
cat .test.tmp.txt | wc -l
uniq .test.tmp.txt | wc -l'
