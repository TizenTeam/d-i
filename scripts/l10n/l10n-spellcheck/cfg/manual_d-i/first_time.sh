svn co svn://anonymous@svn.debian.org/svn/d-i/trunk/installer/doc/manual/po $HOME/di_manual


mkdir $HOME/public_html/spellcheck/manual_d-i
cd $HOME/public_html/spellcheck/manual_d-i

HERE=$(pwd)
mkdir history 1 2

ln -s $HERE/1 previous
ln -s $HERE/2 latest

echo "9999 en 0" >> history/stats_20050320.txt
echo "9999 el 0" >> history/stats_20050320.txt
echo "9999 pt 0" >> history/stats_20050320.txt
echo "9999 ru 0" >> history/stats_20050320.txt
echo "9999 tl 0" >> history/stats_20050320.txt

cp history/stats_20050320.txt previous/stats.txt
mkdir previous/nozip
mkdir previous/zip

cp history/stats_20050320.txt latest/stats.txt
mkdir latest/nozip
mkdir latest/zip
