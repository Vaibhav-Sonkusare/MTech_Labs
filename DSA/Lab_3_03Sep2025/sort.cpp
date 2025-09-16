#include<bits/stdc++.h>

using namespace std;
void merge(int l, int h,int a1[], int a2[] ,int arr[]){
    int i=0;
    int j=0;
    int x=l;
    int mid=(l+h)/2;
    int n=mid-l+1;
    int m=h-mid;
    bool f;
    if(l%2==0){
        f=true;
    }
    else f=false;
    while(i<n && j<m){
        if(f){
        if(a1[i]<a2[j]){
            arr[x++]=a1[i++];
        }
        else{
            arr[x++]=a2[j++];
        }
    }
    else{
        if(a1[i]>a2[j]){
            arr[x++]=a1[i++];
        }
        else{
            arr[x++]=a2[j++];
        }
    }

    f=!f;

    }
    while(i<n){
        arr[x++]=a1[i++];
    }
    while(j<m){
        arr[x++]=a2[j++];
    }
}
void sort(int l, int h, int arr[]){
    if(l<h){
        int mid=(h+l)/2;
        sort(l,mid,arr);
        sort(mid+1,h,arr);
        int a1[mid-l+1]; 
        int a2[h-mid];
        int x=0;
        int y=0;
        for(int i=l;i<=mid;i++){
            a1[x++]=arr[i];
        }
        for(int i=mid+1;i<=h;i++){
            a2[y++]=arr[i];
        }
        for(auto num : a1){
        cout<<num<<" ";
    }
    cout<<endl;
    for(auto num : a2){
        cout<<num<<" ";
    }

    cout<<endl;
        merge(l,h,a1,a2,arr);
        for(int i=l;i<=h;i++)
        cout<<arr[i];
        cout<<endl;
    }
}

int main(){
    int arr[]={9,8,7,6,5,4,3,2,1};
    
    sort(0,8,arr);

    for(auto num : arr){
        cout<<num<<" ";
    }
    return 0;
}

