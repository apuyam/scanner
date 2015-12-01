#define FEE 2.50

struct payload_struct {
	int cid;
	char dev;
	char valid;
	float balance;
	char timestamp[15];
};
typedef struct payload_struct payload;