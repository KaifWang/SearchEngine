CXX = g++
CXXFLAGS = -std=c++17 -g -Wall
SSLFLAGS = -L/usr/local/opt/openssl/lib  -lssl -lcrypto -lz 
PTHREAD_FLAG = -pthread

all:
	make crawler
	make client
	make server
	chmod +x count
	chmod +x clean_output

killall:
	pkill -u ubuntu

# Main Functions
crawler: crawler.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) ${PTHREAD_FLAG} -o crawler.o

client: ./client_server_model/client.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) -o client.o

server: ./client_server_model/server.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) ${PTHREAD_FLAG} -o server.o

signal_end: ./client_server_model/signal_end.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) -o signal_end.o

delete: delete_frontier.cpp
	$(CXX) $(CXXFLAGS) $^ -o delete.o

# Run Server Helper
count:
	ls ./outputs/parsed_output/ | wc -l

# Privatize key
key:
	sudo chmod 600 ./aws_keys/eecs440_zwenxuan.pem
	sudo chmod 600 ./aws_keys/eecs440_kaifan.pem
	sudo chmod 600 ./aws_keys/eecs440_xiaosx.pem
	sudo chmod 600 ./aws_keys/eecs440_leoyan.pem
	sudo chmod 600 ./aws_keys/eecs440_patsu.pem
	sudo chmod 600 ./aws_keys/eecs440_sky.pem

# Machines
m0:
	ssh -i "./aws_keys/eecs440_zwenxuan.pem" ubuntu@ec2-54-175-79-5.compute-1.amazonaws.com

copy_output_m0:
	scp -i "./aws_keys/eecs440_zwenxuan.pem" ubuntu@ec2-54-175-79-5.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output0.txt

m1:
	ssh -i "./aws_keys/eecs440_zwenxuan.pem" ubuntu@ec2-52-90-84-173.compute-1.amazonaws.com

copy_output_m1:
	scp -i "./aws_keys/eecs440_zwenxuan.pem" ubuntu@ec2-52-90-84-173.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output1.txt

m2:
	ssh -i "./aws_keys/eecs440_leoyan.pem" ubuntu@ec2-44-192-49-252.compute-1.amazonaws.com

copy_output_m2:
	scp -i "./aws_keys/eecs440_leoyan.pem" ubuntu@ec2-44-192-49-252.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output2.txt

m3:
	ssh -i "./aws_keys/eecs440_leoyan.pem" ubuntu@ec2-34-204-204-199.compute-1.amazonaws.com

copy_output_m3:
	scp -i "./aws_keys/eecs440_leoyan.pem" ubuntu@ec2-34-204-204-199.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output_mini.txt output3.txt

m4:
	ssh -i "./aws_keys/eecs440_patsu.pem" ubuntu@ec2-3-238-239-55.compute-1.amazonaws.com

copy_output_m4:
	scp -i "./aws_keys/eecs440_patsu.pem" ubuntu@ec2-3-238-239-55.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output4.txt

m5:
	 ssh -i "./aws_keys/eecs440_patsu.pem" ubuntu@ec2-3-235-224-45.compute-1.amazonaws.com

copy_output_m5:
	scp -i "./aws_keys/eecs440_patsu.pem" ubuntu@ec2-3-235-224-45.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output5.txt

m6:
	ssh -i "./aws_keys/eecs440_sky.pem" ubuntu@ec2-3-83-185-174.compute-1.amazonaws.com

copy_output_m6:
	scp -i "./aws_keys/eecs440_sky.pem" ubuntu@ec2-3-83-185-174.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output6.txt

m7:
	ssh -i "./aws_keys/eecs440_sky.pem" ubuntu@ec2-3-239-50-172.compute-1.amazonaws.com

copy_output_m7:
	scp -i "./aws_keys/eecs440_sky.pem" ubuntu@ec2-3-239-50-172.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output7.txt

m8:
	ssh -i "./aws_keys/eecs440_kaifan.pem" ubuntu@ec2-3-235-146-141.compute-1.amazonaws.com

copy_output_m8:
	scp -i "./aws_keys/eecs440_kaifan.pem" ubuntu@ec2-3-235-146-141.compute-1.amazonaws.com:/home/ubuntu/data/crawler/output.txt output8.txt

m9:
	ssh -i "./aws_keys/eecs440_kaifan.pem" ubuntu@ec2-34-231-110-51.compute-1.amazonaws.com

copy_output_m9:
	scp -i "./aws_keys/eecs440_kaifan.pem" ubuntu@ec2-34-231-110-51.compute-1.amazonaws.com:/home/ubuntu/big/crawler/output.txt output9.txt

# machine 0
xiaosx0:
	ssh -i "./aws_keys/eecs440_xiaosx.pem" ubuntu@ec2-13-58-74-142.us-east-2.compute.amazonaws.com

end_xiaosx0:
	./signal_end.o ec2-13-58-74-142.us-east-2.compute.amazonaws.com 5000 5002

copy_output_xiaosx0:
	scp -i "./aws_keys/eecs440_xiaosx.pem" ubuntu@ec2-13-58-74-142.us-east-2.compute.amazonaws.com:/home/ubuntu/crawler/output.txt output_xiaosx.txt

# Clean Output
clean_output:
	rm -rf outputs
	mkdir outputs
	mkdir -p outputs/parsed_output
	mkdir -p outputs/download_output
	touch outputs/parsed_output/file.placeholder
	touch outputs/download_output/file.placeholder
	rm checkpoints/checkalive.txt
	touch checkpoints/checkalive.txt
	rm checkpoints/seenset.txt
	touch checkpoints/seenset.txt

clean:
	rm *.o
	rm -rf *.dSYM

sync:
	make clean_output
	make clean
	git restore checkpoints/checkalive.txt
	git restore checkpoints/frontier.txt
	git pull
	make all

# Unit Tests
unit_test_network_utility: ./tests/unit_test_network_utility.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) ${PTHREAD_FLAG} -o unit_test_network_utility.o

unit_test_url_parser: ./tests/unit_test_url_parser.cpp
	$(CXX) $(CXXFLAGS) $^ -o unit_test_url_parser.o

unit_test_frontier: ./tests/unit_test_frontier.cpp
	$(CXX) $(CXXFLAGS) $^ ${PTHREAD_FLAG} -o unit_test_frontier.o

unit_test_frontier_simple: ./tests/unit_test_frontier_simple.cpp
	$(CXX) $(CXXFLAGS) $^ ${PTHREAD_FLAG} -o unit_test_frontier_simple.o

unit_test_html_parser: ./tests/unit_test_html_parser.cpp
	$(CXX) $(CXXFLAGS) $^ -o unit_test_html_parser.o

unit_test_priority_queue_iterate: ./tests/unit_test_priority_queue_iterate.cpp
	$(CXX) $(CXXFLAGS) $^ -o unit_test_priority_queue_iterate.o

unit_test_url_fixer: ./tests/unit_test_url_fixer.cpp
	$(CXX) $(CXXFLAGS) $^ -o unit_test_url_fixer.o

test: test.cpp
	$(CXX) $(CXXFLAGS) $^ ${PTHREAD_FLAG} -o test.o

# Helper Service
server_class: ./client_server_model/server_class.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) ${PTHREAD_FLAG} -o server_class.o

processor_killer: ./client_server_model/processor_killer.cpp
	$(CXX) $(CXXFLAGS) $^ $(SSLFLAGS) ${PTHREAD_FLAG} -o processor_killer.o

processor_killer_mtx: ./client_server_model/processor_killer_mtx.cpp
	$(CXX) $(CXXFLAGS) $^ ${PTHREAD_FLAG} -o processor_killer_mtx.o

# Send Request
# Note: do not remove duplicsation from list below. Duplication in send list is intentional
TARGET_SERVER_ADDR0 = ec2-54-175-79-5.compute-1.amazonaws.com
TARGET_SERVER_ADDR1 = ec2-54-175-79-5.compute-1.amazonaws.com

send_client_request11:
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.soundcloud.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.pinterest.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.aliexpress.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.businessinsider.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.foxnews.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.android.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.nbcnews.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.disney.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.wikihow.com/Main-Page/Main-Page
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://stackoverflow.com/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.enable-javascript.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.shopify.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.alibaba.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.geeksforgeeks.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://zh.wikipedia.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.cplusplus.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.lsa.umich.edu/cg//
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.mit.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.sciencedirect.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.nginx.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.cbc.ca
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.apache.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.hp.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.mysql.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.target.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.pku.edu.cn
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.tsinghua.edu.cn/index.htm
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.sjtu.edu.cn

send_client_request12:
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.spotify.com/us/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://finance.yahoo.com/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.xbox.com:443/en-US/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.netflix.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.quora.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.britannica.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.loc.gov
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.ikea.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.umich.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.metro.co.uk
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.yelp.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.cornell.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.hollywoodreporter.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.washington.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.huawei.com/en/?ic_medium=direct&ic_source=surlent
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.ibm.com/us-en/?ar=1
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.hm.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://arxiv.org/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.vox.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.billboard.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.state.gov
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.scientificamerican.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.fda.gov
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.utexas.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.cia.gov

send_client_request13:
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.cmu.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.airbnb.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.answers.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.yale.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://sports.yahoo.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.usgs.gov
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.khanacademy.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.newyorker.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.fifa.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://ece.engin.umich.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://bulletin.engin.umich.edu/courses/eecs
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.debian.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.wiktionary.org
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.standard.co.uk/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.ca.gov
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.udemy.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.canva.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.dell.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://eltahir.mit.edu/books/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://bcs.mit.edu/user/login
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://eltahir.mit.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://illinois.edu
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.forbes.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://translate.google.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.ups.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.usps.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.fedex.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.sf-express.com/cn/sc
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.sf-international.com/us/en/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.paypal.com/us/home
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.zillow.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.bankofamerica.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.wellsfargo.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.chase.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.hulu.com/welcome
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.costco.com/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.indeed.com/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.outlook.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.walgreens.com
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://fortune.com/fortune500/
	./client.o $(TARGET_SERVER_ADDR1) 5000 https://www.nasdaq.com

send_client_request01:
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.youtube.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.google.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.apple.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.microsoft.com/en-us/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.blogger.com/about/?bpli=1
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.linkedin.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://support.google.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.cloudflare.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.wordpress.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.adobe.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://en.wikipedia.org/wiki/Main_Page
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.mozilla.org/en-US/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://about.google/intl/ALL_us/products/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://vimeo.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://line.me/en/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://europa.eu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://bp.blogspot.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.bbc.co.uk/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.amazon.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.cnn.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.facebook.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://vk.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://weather.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://outlook.live.com/owa/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.yahoo.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.nytimes.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.t.me
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.slideshare.net
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://issuu.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://developers.google.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.jimdofree.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.reuters.com

send_client_request02:
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.imdb.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.theguardian.com/us/us
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.myspace.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://mail.ru/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.dailymotion.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.brandbucket.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.bbc.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.w3.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.w3.org/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.globo.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.paypal.com/us/home
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.forbes.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://creativecommons.org/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.msn.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.opera.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.weebly.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.wikimedia.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.nih.gov
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.dropbox.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.harvard.edu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.time.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.nasa.gov
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.office.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.latimes.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.who.int
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://store.steampowered.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://news.yahoo.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.change.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.gravatar.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.un.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.huffingtonpost.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.fandom.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.dailymail.co.uk/ushome/index.html
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://twitter.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.ft.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.amazon.co.jp
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.networkadvertising.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.independent.co.uk/us
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.books.google.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.cpanel.net/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.cdc.gov
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.wsj.com

send_client_request03:
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.reddit.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://edition.cnn.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://wordpress.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.stanford.edu 
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://web.mit.edu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.harvard.edu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://umich.edu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.imdb.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://twitter.com/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.walmart.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.macys.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.sephora.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.expedia.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.mediafire.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.ebay.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.youronlinechoices.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.dan.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://archive.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.buydomains.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.scribd.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.usatoday.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.huffpost.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.gov.uk
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.bloomberg.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://ok.ru/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.plesk.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.samsung.com/us/
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.namecheap.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.wikipedia.org
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.washingtonpost.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.wikia.com
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.berkeley.edu
	./client.o $(TARGET_SERVER_ADDR0) 5000 https://www.amazon.com
