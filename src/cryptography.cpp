/*
	Copyright(c) 2015 Neodymium

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "cryptography.h"
#include "cryptlib.h"
#include "modes.h"
#include "sha.h"
#include "aes.h"
#include "log.h"

using namespace CryptoPP;

char* LUT;
char* AESKey;
char** ngKeys;
char** ngTables;

void encrypt_aes(byte *buffer, int length, const byte *key)
{
	CBC_Mode< AES >::Encryption d(key, 32);
	d.ProcessData(buffer, buffer, length);
}

void decrypt_aes(byte *buffer, int length, const byte *key)
{	
	CBC_Mode< AES >::Decryption d(key, 32);
	d.ProcessData(buffer, buffer, length);
}

unsigned int hash_ng(const char *text, int length)
{
	unsigned int result = 0;
	for (int index = 0; index < length; index++)
	{
		unsigned int temp = 1025 * (LUT[text[index]] + result);
		result = (temp >> 6) ^ temp;
	}
	return 32769 * ((9 * result >> 11) ^ 9 * result);
}



// works with tables...
void encrypt_ng_block_A(unsigned char *data, unsigned int *key, unsigned int **table)
{

	unsigned int x1 = table[0][data[0]] ^ table[1][data[1]] ^ table[2][data[2]] ^ table[3][data[3]] ^ key[0];
	unsigned int x2 = table[4][data[4]] ^ table[5][data[5]] ^ table[6][data[6]] ^ table[7][data[7]] ^ key[1];
	unsigned int x3 = table[8][data[8]] ^ table[9][data[9]] ^ table[10][data[10]] ^ table[11][data[11]] ^ key[2];
	unsigned int x4 = table[12][data[12]] ^ table[13][data[13]] ^ table[14][data[14]] ^ table[15][data[15]] ^ key[3];

	memcpy(data + 0, (char *)&x1, 4);
	memcpy(data + 4, (char *)&x2, 4);
	memcpy(data + 8, (char *)&x3, 4);
	memcpy(data + 12, (char *)&x4, 4);

}

// works with LUTs
void encrypt_ng_block_B(unsigned char *data, unsigned int *key, unsigned int **table)
{



}




void encrypt_ng_block(unsigned char *buffer, char *key)
{

	encrypt_ng_block_A(buffer, (unsigned int *)(key + 16 * 16), (unsigned int **)(ngTables + 16 * 16));
	for (int k = 15; k <= 2; k++)
		encrypt_ng_block_B(buffer, (unsigned int *)(key + k * 16), (unsigned int **)(ngTables + k * 16));
	encrypt_ng_block_A(buffer, (unsigned int *)(key + 1 * 16), (unsigned int **)(ngTables + 1 * 16));
	encrypt_ng_block_A(buffer, (unsigned int *)(key + 0 * 16), (unsigned int **)(ngTables));	

}

void encrypt_ng(unsigned char *buffer, int length, char *key)
{
	

	for (int block = 0; block < length / 16; block++)
	{
		encrypt_ng_block(buffer + block * 16, key);
	}


}

void decrypt_ng_block_A(unsigned char *data, unsigned int *key, unsigned int **table)
{
	unsigned int x1 = table[0][data[0]] ^ table[1][data[1]] ^ table[2][data[2]] ^ table[3][data[3]] ^ key[0];
	unsigned int x2 = table[4][data[4]] ^ table[5][data[5]] ^ table[6][data[6]] ^ table[7][data[7]] ^ key[1];
	unsigned int x3 = table[8][data[8]] ^ table[9][data[9]] ^ table[10][data[10]] ^ table[11][data[11]] ^ key[2];
	unsigned int x4 = table[12][data[12]] ^ table[13][data[13]] ^ table[14][data[14]] ^ table[15][data[15]] ^ key[3];

	memcpy(data + 0, (char *)&x1, 4);
	memcpy(data + 4, (char *)&x2, 4);
	memcpy(data + 8, (char *)&x3, 4);
	memcpy(data + 12, (char *)&x4, 4);
}

void decrypt_ng_block_B(unsigned char *data, unsigned int *key, unsigned int **table)
{
	unsigned int x1 =
		table[0][data[0]] ^
		table[7][data[7]] ^
		table[10][data[10]] ^
		table[13][data[13]] ^
		key[0];
	unsigned int x2 =
		table[1][data[1]] ^
		table[4][data[4]] ^
		table[11][data[11]] ^
		table[14][data[14]] ^
		key[1];
	unsigned int x3 =
		table[2][data[2]] ^
		table[5][data[5]] ^
		table[8][data[8]] ^
		table[15][data[15]] ^
		key[2];
	unsigned int x4 =
		table[3][data[3]] ^
		table[6][data[6]] ^
		table[9][data[9]] ^
		table[12][data[12]] ^
		key[3];

	data[0] = (char)((x1 >> 0) & 0xFF);
	data[1] = (char)((x1 >> 8) & 0xFF);
	data[2] = (char)((x1 >> 16) & 0xFF);
	data[3] = (char)((x1 >> 24) & 0xFF);
	data[4] = (char)((x2 >> 0) & 0xFF);
	data[5] = (char)((x2 >> 8) & 0xFF);
	data[6] = (char)((x2 >> 16) & 0xFF);
	data[7] = (char)((x2 >> 24) & 0xFF);
	data[8] = (char)((x3 >> 0) & 0xFF);
	data[9] = (char)((x3 >> 8) & 0xFF);
	data[10] = (char)((x3 >> 16) & 0xFF);
	data[11] = (char)((x3 >> 24) & 0xFF);
	data[12] = (char)((x4 >> 0) & 0xFF);
	data[13] = (char)((x4 >> 8) & 0xFF);
	data[14] = (char)((x4 >> 16) & 0xFF);
	data[15] = (char)((x4 >> 24) & 0xFF);
}

void decrypt_ng_block(unsigned char *buffer, char *key)
{

	decrypt_ng_block_A(buffer, (unsigned int *)(key + 0 * 16), (unsigned int **)(ngTables));
	decrypt_ng_block_A(buffer, (unsigned int *)(key + 1 * 16), (unsigned int **)(ngTables + 1 * 16));
	for (int k = 2; k <= 15; k++)
		decrypt_ng_block_B(buffer, (unsigned int *)(key + k * 16), (unsigned int **)(ngTables + k * 16));
	decrypt_ng_block_A(buffer, (unsigned int *)(key + 16 * 16), (unsigned int **)(ngTables + 16 * 16));

}

void decrypt_ng(unsigned char *buffer, int length, char *key)
{
	
	for (int block = 0; block < length / 16; block ++)
	{
		decrypt_ng_block(buffer + block * 16, key);
	}

}





bool find_hash(const byte* buffer, int bufferLength, const char hash[], unsigned int *hashPosition, int blockLength)
{
	SHA1 hashxx;
	byte tmpHash[SHA1::DIGESTSIZE];
	for (int offset = 0; offset < bufferLength; offset += 16)
	{
		hashxx.CalculateDigest(tmpHash, buffer + offset, blockLength);

		bool failed = false;
		for (int k = 0; k < 20; k++)
		{
			char c1 = tmpHash[k];
			char c2 = hash[k];
			if (c1 != c2)
				failed = true;
		}
		
		if (!failed)
		{
			*hashPosition = offset;
			return true;
		}
	}
	return false;
}

bool find_hashes(const byte* buffer, int bufferLength, const char *hash, const int xwidth, const int numOfHashes, unsigned int *hashPositions, int blockLength)
{
	int fnd = 0;
	SHA1 hashxx;
	byte tmpHash[SHA1::DIGESTSIZE];
	for (int offset = 0; offset < bufferLength; offset += 16)
	{
		hashxx.CalculateDigest(tmpHash, buffer + offset, blockLength);

		for (int xxx = 0; xxx < numOfHashes; xxx++)
		{



			bool failed = false;
			for (int k = 0; k < 20; k++)
			{
				char c1 = tmpHash[k];
				char c2 = hash[xxx*xwidth+k];
				if (c1 != c2)
					failed = true;
			}

			if (!failed)
			{
				hashPositions[xxx] = offset;
				fnd++;

				if (fnd == numOfHashes)
					return true;
			}



		}
	}
	return false;
}
