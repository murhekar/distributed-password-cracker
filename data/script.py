# Generates random strings and their hashes 
import crypt, sys, string, random
def passwd_gen(size = 7, chars = string.digits):
	return ''.join(random.choice(chars) for _ in range(size))
for i in range(0,10) :
	num = passwd_gen()
	print(num)	
	print(crypt.crypt(num, "aa"))
