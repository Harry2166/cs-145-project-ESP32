from django.shortcuts import render, redirect
from .models import Message

def client_view(request, client):
	if client not in ['a', 'b']:
		return redirect('/a')  # Default to Client A's view

	sender = client.upper()
	receiver = 'B' if sender == 'A' else 'A'

	if request.method == 'POST':
		content = request.POST.get('content')
		if content:
			Message.objects.create(sender=sender, receiver=receiver, content=content)
			return redirect(f'/{client}')

	messages = Message.objects.filter(sender=sender) | Message.objects.filter(receiver=sender)
	messages = messages.order_by('timestamp')

	return render(request, 'chat/client_view.html', {'client': client, 'messages': messages})

# Create your views here.
