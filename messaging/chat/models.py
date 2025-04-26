from django.db import models

class Message(models.Model):
	sender = models.CharField(max_length=1)  # 'A' or 'B'
	receiver = models.CharField(max_length=1)  # 'A' or 'B'
	content = models.TextField()
	timestamp = models.DateTimeField(auto_now_add=True)

	def __str__(self):
		return f"From {self.sender} to {self.receiver}: {self.content[:20]}"
