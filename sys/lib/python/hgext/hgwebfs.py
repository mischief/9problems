''' webfs support '''

import mercurial.url
import re

class Webconn:
	def __init__(self, mnt, req):
		ctl = open(mnt+'/clone', 'r+', 0)
		try:
			self.url = req.get_full_url()
			self.dir = mnt+'/'+ctl.readline().rstrip('\n')
			ctl.seek(0)
			ctl.write('url '+self.url)
			m = ''
			for h in req.headers:
				m += h+': '+req.headers[h]+'\r\n'
			if len(m) > 0:
				m = 'headers '+m
				print m
				ctl.seek(0)
				ctl.write(m)
			if req.has_data():
				data = req.get_data()
				post = open(self.dir+'/postdata', 'w', 0);
				try:
					while True:
						buf = data.read(4096)
						if len(buf) == 0:
							break
						post.write(buf)
				finally:
					post.close()
			self.body = open(self.dir+'/body', 'r', 0)
		finally:	
			ctl.close()

	def read(self, amt=4096):
		return self.body.read(amt);

	def close(self):
		self.body.close()
		self.body = None
		self.dir = None

	def geturl(self):
		return self.url

	def getheader(self, key):
		name = re.sub(r'[^a-z]+', '', key.lower())
		try:
			f = open(self.dir+'/'+name, 'r', 0)
			try:
				hdr = f.read()
			finally:
				f.close()
			return hdr
		except:
			return None

class Webopener:
	def __init__(self):
		self.handlers = []

	def open(self, req, data=None):
		return Webconn('/mnt/web', req)

	def close(self):
		pass


def webopener(ui, authinfo=None):
	return Webopener();

mercurial.url.opener = webopener
