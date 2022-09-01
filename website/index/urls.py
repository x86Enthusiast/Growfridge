from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='index'), #put the index at the root of the project
    path('requestVals/', views.requestVals, name='requestVals'),
    path('js/', views.index, name='js'),
    path('resources/', views.index, name='resources'),
    path('html/', views.index, name='html'),
    path('getAuth/', views.getAuth, name='getAuth'),
    path('checkAuth/', views.checkAuth, name='checkAuth'),
]