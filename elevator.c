#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include<windows.h>

pthread_t thread1;//엘리베이터 상태 변수 값들을 지정해주는 쓰레드 변수
pthread_t thread2;//엘리베이터의 맵을 계속 출력해주기 위한 쓰레드 변수
pthread_t thread3;//온도 제어를 위한 쓰레드 함수
pthread_mutex_t mutex_lock=PTHREAD_MUTEX_INITIALIZER;
 
typedef enum {UP,DOWN,STOP} VECTOR;//벡터의 방향의 자료형을 미리 만들어줌
typedef enum {ON,OFF} SWITCH;//스위치 변수의 자료형을 미리 만들어줌
struct elevator{
	int height;//엘리베이터의 높이를 나타내기 위해서 만들었습니다. 구조체 포인터 배열로 선언해서, 1층이 height=19를 나타나게 됩니다.그래서 표현할때 (20-height) 방식으로 했음
	int destination[8];//목적지를 array 배열로 만들었습니다. 그래서 선입후출 구조를 사용해 목적지 포인터 변수로 따로 입/출구를 가리키게 표현.
	int destination_pointer;//목적지 배열의 마지막 index의 다음 index를 가리키게끔 해놨습니다.그래서 목적지를 입력할 때 목적지 pointer 변수로 index에 넣게끔 함
	int waiting_time;//대기 시간을 설정했습니다.0초에서 시작해서 2초가 되었을 때, 대기 시간을 멈춫게 함(승하차 시간을 고려)
	int temperature;//현재 엘리베이터의 온도 저장 변수
	SWITCH flag;//사용 엘리베이터 없을 시, 이를 제어하기 위한 스위치 변수
	SWITCH waiting_on_off;//대기하고 있는지의 여부를 나타내주기 위한 스위치 변수
	SWITCH on_off;//엘리베이터가 명령을 받는지 안받는지를 나타내기 위해 선언(주로 입력을 받을 시에만 움직이게끔 했습니다)
	VECTOR vector;//방향 상태를 저장하기 위한 변수입니다.
	VECTOR temperature_vector;//온도의 방향 상태 저장 변수
	
};

struct elevator el[6];//엘리베이터 구조체 배열 입니다(0~1:1~10층, 2~3:11층~20층, 4~5:모든 층)
struct elevator *map[20][6];//엘리베이터 구조체를 가리키는 포인터 배열 입니다. 그래서 가리키는 구조체만 출력을 시키고, null 값일 때는 빈 공백을 출력시킵니다

VECTOR player_vector;//사용자가 올라가는지 내려가기를 원하는지 나타내주는 방향 변수(ex: 1층에서 버튼을 누르고, 10층으로 가고싶다 하면, UP 설정)
char *up_down[2]={"up", "dn"};//벡터 변수의 방향에 따라서, "up", "dn"을 출력
int starting_site;//처음 엘리베이터 층 버튼을 누른 위치를 저장
int destination_site;//가고자 하는 엘리베이터 층을 저장하기 위한 변수
int input=0;//menu 입력 변수입니다.
int min=7;//엘리베이터 구조체들 중에서, starting_site 지점에서 절댓값 함수를 이용하여, 뺀 다음에 최소 거리가 오게끔 하여 그 엘리베이터를 저장시키기 위한 변수
int l=95;//gotoxy 제어 변수
int k=26;//gotoxy 제어 변수
int full_count=0;//대기 시간 저장 변수(3초까지 대기)
int temperature_elevator;//온도를 변경할 엘리베이터 index 저장 변수
SWITCH full_on_off=OFF;//온도가 최대,최소 일 때 변하는 스위치 변수
SWITCH all_stop=OFF;//일시 정지 제어 스위치 변수
SWITCH light_switch=OFF; //엘리베이터 불 전원 제어 스위치 변수
SWITCH temperature_switch=OFF;//온도를 변하게끔 하는 제어 스위치 변수


int select_elevator();//절댓값으로 최소 거리를 구하여, 움직이게끔 할 엘리베이터를 선정해주는 함수입니다.
void option1();//menu 1:엘리베이터를 선턱했을 시, 층수 입력을 받게끔 하는 함수
void option2();//menu 2:일시정지 함수
void option3();//menu 3:엘리베이터 전등 on/off 함수
void option4();//menu 4:온도 조절 함수
void print_marking();//맵 출력할 때 편하게 하기 위해 따로 만들어 놓은 함수
void gotoxy(int p,int q);//gotoxy 제어 함수(x,y) 좌표로 설정을 하고, +x는 오른쪽으로, +y는 아랫쪽으로 가게 됩니다
void *elevator(void *);//엘리베이터 상태 변수들을 조절하는 쓰레드 함수
void *print_current();//엘리베이터의 현재 상태를 계속 출력시켜주는 쓰레드 함수
void *temperature_control();//온도의 up/down 관리 쓰레드 함수
void eraser();//지우개 함수, gotoxy(x,y)를 이용하여 빈 공간출력

int main(void)
{
	int i;
	for(i=0;i<20;i++)//초기화 부분
	{
		for(int j=0;j<6;j++)
		{
			map[i][j]=NULL;//처음에 모든 map을 NULL 값으로 초기화
			el[j].vector=STOP;//처음 방향은 STOP
			el[j].on_off=OFF;//입력을 받을 시에 OFF
			el[j].destination_pointer=0;//처음 index 0을 가리키게 끔 함
			el[j].waiting_time=0;
			el[j].waiting_on_off=OFF;
			el[j].flag=OFF;
			el[j].temperature=23;
			el[j].temperature_vector=STOP;
			if(j<2||j>3)//저층과 모든 층을 움직이는 엘리베이터들은 1층으로 선정(20-height(19)=1층)
			{
				el[j].height=19;
				map[el[j].height][j]=&el[j];
			}
			else//고층 만을 움직이는 엘리베이터 이므로 11층으로 선정(20-height(9)=11층)
			{
				el[j].height=9;
				map[el[j].height][j]=&el[j];
			}
		}
	}

	system("clear");//엘리베이터가 시작하면 화면 clear를 해주고 시작	
	pthread_create(&thread1,NULL,&elevator,NULL);
	pthread_create(&thread2,NULL,&print_current,NULL);
	pthread_create(&thread3,NULL,&temperature_control,NULL);
		
	while(1)
	{
		scanf("%d", &input);//menu 입력값을 받습니다
		if(input<0||input>5){
			gotoxy(l,k);
			printf("  ");
			continue;
		}
		if(input==1)
		{
			eraser();//지우개 함수로, 화면 정리
			option1();//1을 입력받으면 option1이 시작
			min=select_elevator();//움직일 엘리베이터를 선정하여, min 값에 저장
			el[min].on_off=ON;//선정한 엘리베이터가 명령을 받도록 스위치를 ON으로 설정
			el[min].waiting_on_off=ON;
			el[min].vector=STOP;
			l=83;//전역 변수로 gotoxy(x,y) 전용 커서 변수를 이용해서, 전체적인 출력을 제어
			k=27;
			gotoxy(l,k);
			printf("%d번 엘리베이터가 응답했습니다!!",min+1);
			l=95;
			k=26;
			gotoxy(l,k);
			printf("  ");
		}
		if(input==2)//일시정지 메뉴
		{
			eraser();
			option2();
			l=83;
			k=27;
			gotoxy(l,k);
			if(all_stop==ON)
			{
				printf("일시 정지 ON 상태입니다");
			}
			else if(all_stop==OFF)
			{
				printf("일시 정지 OFF상태입니다");
			}
			l=95;
			k=26;
			gotoxy(l,k);
			printf(" ");
		}
		if(input==3)//엘리베이터 전등 메뉴
		{
			eraser();
			option3();
			l=83;
			k=27;
			gotoxy(l,k);
			if(light_switch==ON)
			{
				printf("불이 켜져 있는 상태입니다");
			}
			else if(light_switch==OFF)
			{
				printf("불이 꺼져 있는 상태입니다");
			}
	 		l=95;
			k=26;
			gotoxy(l,k);
			printf("  ");
		}
		if(input==4)//엘리베이터 온도 조절 메뉴
		{
			eraser();
			option4();
			l=83;
			k=27;
			gotoxy(l,k);
			if(el[temperature_elevator-1].temperature_vector==UP)
			{
				printf("%d번 엘리베이터가 히터를 가동했습니다",temperature_elevator);
			}
			else if(el[temperature_elevator-1].temperature_vector==DOWN)
			{
				printf("%d번 엘리베이터가 에어컨을 가동했습니다",temperature_elevator);
			}
			else
			{
				printf("%d번 엘리베이터가 온도를 유지합니다",temperature_elevator);
			}
			l=95;
			k=26;
			gotoxy(l,k);
			printf("   ");
		}
		if(input==5)//종료
		{
			system("clear");
			break;
		}

	}
	
	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);
	pthread_join(thread3,NULL);
	return 0;
}

void gotoxy(int p, int q)
{
	printf("\033[%d;%df", q,p);
	fflush(stdout);
}

void print_marking()
{
	printf("================================================================");//엘리베이터 출력 할시 편하게 하기 위해 이 부분만 따로 함수를 만들었습니다
}	

void *elevator(void *arg)//엘리베이터 상태 제어 쓰레드 함수
{
	while(1)
	{
		if(input==5)
		{
			break;
		}
		if(all_stop==OFF)//일시정지 스위치가 OFF일 때
		{
			if(el[min].vector==STOP&&el[min].on_off==ON)//엘리베이터 탑승 지점에 따라서 경우의 수 분리
			{	
				if(starting_site>(20-el[min].height))//지점 보다 위치가 낮을 때
				{
					el[min].vector=UP;
					el[min].destination[el[min].destination_pointer]=destination_site;
					el[min].destination_pointer++;
					el[min].destination[el[min].destination_pointer]=starting_site;
					el[min].destination_pointer++;	
				}
				else if(starting_site<(20-el[min].height))//지점 보다 위치가 높을 떄
				{
					el[min].vector=DOWN;
					el[min].destination[el[min].destination_pointer]=destination_site;
					el[min].destination_pointer++;
					el[min].destination[el[min].destination_pointer]=starting_site;
					el[min].destination_pointer++;
				}
				else
				{
					if(destination_site>(20-el[min].height))//만일 탑승 지점과 현재 높이가 같을 때는, 목적지로 분리
					{
						el[min].vector=UP;
						el[min].destination[el[min].destination_pointer]=destination_site;
						el[min].destination_pointer++;
						el[min].flag=ON;
					}	
					else if(destination_site<(20-el[min].height))//목적지보다 높을 때
					{
						el[min].vector=DOWN;
						el[min].destination[el[min].destination_pointer]=destination_site;
						el[min].destination_pointer++;
						el[min].flag=ON;
					}
				}
				el[min].on_off=OFF;
			}
			else if(el[min].vector!=STOP&&el[min].on_off==ON&&full_on_off==ON)//움직이고 있는 엘리베이터 일 때
			{
				if(el[min].flag==OFF)//만일 사용 가능한 엘리베이터가 없을 시, 가장 우선순위가 높은(가까운) 엘리베이터를 선택함
				{
					el[min].destination_pointer+=2;
					el[min].destination[el[min].destination_pointer-2]=el[min].destination[el[min].destination_pointer-4];
					el[min].destination[el[min].destination_pointer-1]=el[min].destination[el[min].destination_pointer-3];
					el[min].destination[el[min].destination_pointer-3]=starting_site;
					el[min].destination[el[min].destination_pointer-4]=destination_site;
				}
				else if(el[min].flag==ON)//만일 사용 엘리베이터가 없고, 대기를 받던 사람의 출발지가, 사용중인 엘리베이터의 목적지와 같을시의 경우의 수
				{
					el[min].destination_pointer+=2;
					el[min].destination[el[min].destination_pointer-1]=el[min].destination[el[min].destination_pointer-3];
					el[min].destination[el[min].destination_pointer-2]=starting_site;
					el[min].destination[el[min].destination_pointer-3]=destination_site;
				}
				el[min].on_off=OFF;
				full_on_off=OFF;
				full_count=0;
			}
			else if(el[min].vector!=STOP&&el[min].on_off==ON&&full_on_off==OFF)
			{
				el[min].destination[el[min].destination_pointer]=destination_site;
				el[min].destination_pointer++;
				el[min].destination[el[min].destination_pointer]=starting_site;
				el[min].destination_pointer++;
				el[min].on_off=OFF;
	
			}
			int i=0;
			while(i<6)
			{	
				if(el[i].waiting_on_off==ON)//승하차 중일 때, 잠시 대기(3초까지)
				{
					el[i].waiting_time++;
					if(el[i].waiting_time==3)
					{
						el[i].waiting_on_off=OFF;
						el[i].waiting_time=0;
						if(el[i].destination_pointer!=0)
						{					
							if(el[i].destination[el[i].destination_pointer-1]>(20-el[i].height))
							{
								el[i].vector=UP;
							}
							else if(el[i].destination[el[i].destination_pointer-1]<(20-el[i].height))
							{
								el[i].vector=DOWN;
							}
						}
						else
						{
							el[i].vector=STOP;
						}	
					}
				}
				if(el[i].vector!=STOP)//방향에 따라 엘리베이터 변수의 값 제어
				{
					if(el[i].waiting_on_off==OFF)
					{
						if(el[i].vector==UP)
						{
							el[i].height--;
							map[el[i].height][i]=&el[i];
							map[el[i].height+1][i]=NULL;
						}
						else if(el[i].vector==DOWN)
						{
							el[i].height++;
							map[el[i].height][i]=&el[i];
							map[el[i].height-1][i]=NULL;
						}
					}
					if((20-el[i].height)==el[i].destination[el[i].destination_pointer-1])//최종적으로 도착했을 시
					{
						el[i].waiting_on_off=ON;
						el[i].vector=STOP;
						el[i].destination[el[i].destination_pointer-1]=0;
						el[i].destination_pointer--;
						if(el[i].destination_pointer!=0&&el[i].waiting_on_off==OFF)//목적지에 도착했는데, 최종이 아니고, 사람이 더 타있는 경우
						{
							if(el[i].destination[el[i].destination_pointer-1]>(20-el[i].height))
							{
								el[i].vector=UP;
							}
							else if(el[i].destination[el[i].destination_pointer-1]<(20-el[i].height))
							{
								el[i].vector=DOWN;
							}	
						}
						else if(el[i].destination_pointer==0||el[i].waiting_on_off==OFF)//더이상 목적지가 없을 시 정지 상태
						{
							el[i].vector=STOP;
						}
					}
				}
				i++;
			}
			sleep(1);
		}

	}

}
void *temperature_control()
{
	while(1)
	{
		if(input==5)
		{
			break;
		}
		if(all_stop==OFF)
		{
			int j=0;	
			while(j<6)//온도의 방향에 따라 변수 값 제어
			{
				if(el[j].temperature_vector!=STOP)
				{
					if(el[j].temperature_vector==UP)//온도가 올라갈 때(최고 30, 최저 0도로 제어)
					{
						if(el[j].temperature>=28)
						{
							el[j].temperature=28;
							el[j].temperature_vector=STOP;
						}
						else
						{
							el[j].temperature++;
						}
					}
					else if(el[j].temperature_vector==DOWN)//내려갈 떄
					{
						if(el[j].temperature<=18)
						{
							el[j].temperature=18;
							el[j].temperature_vector=STOP;
						}	
						else
						{
							el[j].temperature--;
						}
					}
					
				}
				j++;
			}
			sleep(2);
		}
	}
}
void *print_current(void *arg)//현재의 맵 상태 출력 쓰레드 함수
{
	int i,j;
	int n=0;
	while(1)
	{	
		if(input==5)
		{
			break;
		}
		pthread_mutex_lock(&mutex_lock);
		gotoxy(83,20);
		printf("-----------M E N U -----------");//메뉴
		gotoxy(83,21);
		printf("1.엘리베이터 이용");
		gotoxy(83,22);
		printf("2.엘리베이터 일시정지/다시 시작");
		gotoxy(83,23);
		printf("3.엘리베이터 Light On/Off");
		gotoxy(83,24);
		printf("4.엘리베이터 온도조절");
		gotoxy(83,25);
		printf("5.종료");
		gotoxy(83,26);
		printf("메뉴 선택 : ");
		gotoxy(l,k);
		
		n=0;
		for(i=0;i<20;i++)//구조체 포인버 태열 사용. 배열의 원소들 중에서 엘리베이터 구조체의 주소를 가리키는 것만 출력시키게 함(가리키지 않았을 시 공백)
		{
			n+=1;
			gotoxy(0,n);
			print_marking();
			n+=1;
			gotoxy(0,n);
			for(j=0;j<6;j++)
			{
				if(map[i][j]==NULL)
				{	
					printf("|       |  ");
				}
				else if(map[i][j]!=NULL&&el[j].vector!=STOP&&el[j].waiting_on_off==OFF)
				{
					if(light_switch==OFF)//엘리베이터 전등 스위치
					{
						if(el[j].destination[el[j].destination_pointer-1]<10)
						{
							printf("| %d층%s |  ",el[j].destination[el[j].destination_pointer-1],up_down[el[j].vector]);
						}
						else
						{
							printf("|%d층%s |  ",el[j].destination[el[j].destination_pointer-1],up_down[el[j].vector]);
						}
					}
					else//전등 스위치 ON 일 때 노란색 출력
					{
						if(el[j].destination[el[j].destination_pointer-1]<10)
						{
							printf("\033[0;33m| %d층%s |  ",el[j].destination[el[j].destination_pointer-1],up_down[el[j].vector]);
							printf("\033[0m");
						}
						else
						{
							printf("\033[0;33m|%d층%s |  ",el[j].destination[el[j].destination_pointer-1],up_down[el[j].vector]);
							printf("\033[0m");
						}
					}
				}
				else if(map[i][j]!=NULL&&el[j].vector==STOP||el[j].waiting_on_off==ON)
				{
					if(light_switch==OFF)
					{
						printf("|  EL   |  ");
					}
					else
					{
						printf("\033[0;33m|  EL   |  ");
						printf("\033[0m");
					}
				}
			}
			printf(" %d층",20-i);
		}
		n+=1;
		gotoxy(0,n);
		print_marking();
		for(int i=0;i<6;i++)//엘리베이터의 상태 기록 출력
		{
			n+=1;
			gotoxy(0,n);
			printf("                                                                                ");
			gotoxy(0,n);
			if(el[i].vector!=STOP&&el[i].waiting_on_off==OFF)//이동 중일 때는 초록색으로 출력
			{
				if(el[i].temperature_vector==DOWN)
				{
					printf("\033[0;32m%d번 엘리베이터가 이동중입니다.\033[0;36m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else if(el[i].temperature_vector==UP)
				{
					printf("\033[0;32m%d번 엘리베이터가 이동중입니다.\033[0;31m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else
				{
					printf("\033[0;32m%d번 엘리베이터가 이동중입니다.\033[0m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
			}
			else if(el[i].vector==STOP&&el[i].waiting_on_off==OFF)//대기 상태 일 때는 일반적으로 하얀색
			{
				if(el[i].temperature_vector==DOWN)
				{
					printf("%d번 엘리베이터는 대기중입니다.\033[0;36m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else if(el[i].temperature_vector==UP)
				{
					printf("%d번 엘리베이터는 대기중입니다.\033[0;31m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else
				{
					printf("%d번 엘리베이터는 대기중입니다.(온도: %d)",i+1,el[i].temperature);
				}
			}
			else if(el[i].waiting_on_off==ON)
			{
				if(el[i].temperature_vector==DOWN)//승하차 중일 때는 연보라색
				{
					printf("\033[0;35m%d번 엘리베이터는 승하차중입니다.\033[0;36m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else if(el[i].temperature_vector==UP)
				{
					printf("\033[0;35m%d번 엘리베이터는 승하차중입니다.\033[0;31m(온도: %d)",i+1,el[i].temperature);
					printf("\033[0m");
				}
				else
				{
					printf("\033[0;35m%d번 엘리베이터는 승하차중입니다.\033[0m(온도: %d)",i+1,el[i].temperature);
				}
			}
			else
			{
				printf("엘리베이터 상태 출력 중 exception!!");
			}
		}
		pthread_mutex_unlock(&mutex_lock);
		gotoxy(l,k);
		sleep(1);
	}
}

int select_elevator()//움직일 엘리베이터를 선택해주는 함수(기준에서 각 엘리베이터의 높이를 뺀 뒤에, 절대값 비교 후 가장 가까운 것이 선정 됨)
{
	int min_value=0;
	int i=min_value+1;
	
	if(starting_site<11&&destination_site<11)//저층 일 때
	{
		while(i<6)
		{
			if(abs(starting_site-(20-el[min_value].height))>abs(starting_site-(20-el[i].height)))//절댓값 비교
			{
				if(el[i].waiting_on_off==OFF)//대기중이 아닐 때
				{	
					if(el[i].vector==STOP)//방향이 정지이면, 무조건 움직임
					{
						min_value=i;
					}
					else if(el[i].vector!=STOP)
					{
						if(starting_site>(20-el[i].height))
						{
							if(el[i].vector==UP&&player_vector==UP)//승객의 방향에 따라서 엘리베이터의 역주행 방지
							{
								min_value=i;
							}
						}
						else if(starting_site<(20-el[i].height))
						{
							if(el[i].vector==DOWN&&player_vector==DOWN)
							{
								min_value=i;
							}
						}
					}
				}
			}
			i++;
			if(i==2)//고층 엘리베이터(index=2,3)을 건너뛰기 위해서
			{
				i+=2;
			}	
		}
		for(int j=0;j<6;j++)
		{
			if(j==2)
			{
				j+=2;
			}
			if(el[j].vector!=STOP)
			{
				full_count++;
			}
			if(full_count==4)
			{
				full_on_off=ON;
			}
		}
	}	
	else if(starting_site>=11&&destination_site>=11)//고층일 떄
	{
		min_value=2;
		i=min_value+1;
		while(i<6)
		{	
			if(abs(starting_site-(20-el[min_value].height))>abs(starting_site-(20-el[i].height)))
			{
				
				if(el[i].waiting_on_off==OFF)
				{
					if(el[i].vector==STOP)
					{
						min_value=i;
					}
					else if(el[i].vector!=STOP)
					{
						if(starting_site>(20-el[i].height))
						{
							if(el[i].vector==UP&&player_vector==UP)
							{
								min_value=i;
							}
						}
						else if(starting_site<(20-el[i].height))
						{
							if(el[i].vector==DOWN&&player_vector==DOWN)
							{
								min_value=i;
							}
						}
					}
				}
			}
			i++;
		}
		for(int j=2;j<6;j++)
		{
			if(el[j].vector!=STOP)
			{
				full_count++;
			}
			if(full_count==4)
			{
				full_on_off=ON;
			}
		}
	}	
	else if(starting_site<11&&destination_site>=11)//모든 층 엘리베이터 일 때
	{
		min_value=4;
		i=min_value+1;
		while(i<6)
		{	
			if(abs(starting_site-(20-el[min_value].height))>abs(starting_site-(20-el[i].height)))
			{
				if(el[i].waiting_on_off==OFF)
				{
					if(el[i].vector==STOP)
					{
						min_value=i;
					}
					else if(el[i].vector!=STOP)
					{
						if(starting_site>(20-el[i].height))
						{
							if(el[i].vector==UP&&player_vector==UP)
							{
								min_value=i;
							}
						}
						else if(starting_site<(20-el[i].height))
						{
							if(el[i].vector==DOWN&&player_vector==DOWN)
							{
								min_value=i;
							}
						}
					}
				}
			}
			i++;
		}
		for(int j=4;j<6;j++)
		{
			if(el[j].vector!=STOP)
			{
				full_count++;
			}
			if(full_count==2)
			{
				full_on_off=ON;
			}
		}
	}	
	else if(starting_site>=11&&destination_site<11)//모든 충 움직이는 엘리베이터 일 때
	{
		min_value=4;
		i=min_value+1;
		while(i<6)
		{	
			if(abs(starting_site-(20-el[min_value].height))>abs(starting_site-(20-el[i].height)))
			{
				if(el[i].waiting_on_off==OFF)
				{
					if(el[i].vector==STOP)
					{
						min_value=i;
					}
					else if(el[i].vector!=STOP)
					{
						if(starting_site>(20-el[i].height))
						{
							if(el[i].vector==UP&&player_vector==UP)
							{
								min_value=i;
							}
	
						}
						else if(starting_site<(20-el[i].height))
						{
							if(el[i].vector==DOWN&&player_vector==DOWN)
							{
								min_value=i;
							}
						}
					}
				}
			}
			i++;
		}
		for(int j=4;j<6;j++)
		{
			if(el[j].vector!=STOP)
			{
				full_count++;
			}
			if(full_count==2)
			{
				full_on_off=ON;
			}
		}
	}


	return min_value;
}
void option1()//메뉴 1 함수
{
	while(1) 
 	{
		l=83;
		k=27;
                gotoxy(l,k);
                printf("현재 층수를 입력: ");
		l=101;
		gotoxy(l,k);
                scanf("%d",&starting_site);
                if(starting_site>20 || starting_site<1)
		{
			gotoxy(l,k);
			printf("  ");
			continue;
		}
                else
		{	
			l=83;
			gotoxy(l,k);
			printf("                                    ");
			break;
		}
        }
        while(1)
        {
		l=83;
                gotoxy(l,k);
		printf("가고자 하는  층수를 입력: ");
		l=109;
		gotoxy(l,k);
                scanf("%d",&destination_site);
                if(destination_site>20 || destination_site<1 || destination_site == starting_site)
		{
			gotoxy(l,k);
			printf("  ");
			continue;
		}
                else 
		{
			l=83;
			gotoxy(l,k);
			printf("                                              ");
			l=95;
			k=26;
			gotoxy(l,k);
			printf("   ");
			break;
		}
        }
	if(starting_site>destination_site)//플레이어가 향하는 방향을 나타냄(ex: 1층에서 3층으로 간다면→UP을 나타냄!!)
	{
		player_vector=DOWN;
	}
	else if(starting_site<destination_site)
	{
		player_vector=UP;
	}
}
void option2()//메뉴 2 함수
{
	while(1)
	{
		int stop_button;
		l=83;
		k=27;
		gotoxy(l,k);
		printf("일시 정지(2를 입력하면 ON/OFF): ");
		l=115;
		gotoxy(l,k);
		scanf("%d", &stop_button);
		if(stop_button!=2)
		{
			gotoxy(l,k);
			printf("   ");
			continue;
		}
		else
		{
			if(all_stop==ON)
			{
				all_stop=OFF;
			}
			else if(all_stop==OFF)
			{
				all_stop=ON;
			}
		}
		l=83;
		k=27;
		gotoxy(l,k);
		printf("                                                 ");
		break;
	}
}
void option3()//메뉴 3 함수
{
	while(1)
	{
		int light_button;
		l=83;
		k=27;
		gotoxy(l,k);
		printf("Light On/Off(2를 입력하시오): ");
		l=115;
		gotoxy(l,k);
		scanf("%d",&light_button);
		if(light_button!=2)
		{
			gotoxy(l,k);
			printf("  ");
			continue;
		}
		else
		{
			if(light_switch==ON)
			{
				light_switch=OFF;
			}
			else if(light_switch==OFF)
			{
				light_switch=ON;
			}
		}
		l=83;
		k=27;
		gotoxy(l,k);
		printf("                                             ");
		break;
	}
}
void option4()//메뉴 4 함수
{
	int temperature_button;
	while(1)
	{
		l=83;
		k++;
		gotoxy(l,k);
		printf("입력 할 엘리베이터를 고르시오: ");
		l=114;
		gotoxy(l,k);
		scanf("%d", &temperature_elevator);
		if(temperature_elevator<1||temperature_elevator>6)
		{
			gotoxy(l,k);
			printf("  ");
			continue;
		}
		else
		{
			l=83;
			gotoxy(l,k);
			printf("                                       ");
			break;
		}
		
	}
	while(1)
	{
		l=83;
		gotoxy(l,k);
		printf("버튼 입력(1:히터,2:에어컨,3:정지): ");
		l=118;
		gotoxy(l,k);
		scanf("%d", &temperature_button);
		if(temperature_button<1||temperature_button>3)
		{
			gotoxy(l,k);
			printf("  ");
			continue;
		}
		else
		{
			if(temperature_button==1)
			{
				el[temperature_elevator-1].temperature_vector=UP;
			}
			else if(temperature_button==2)
			{
				el[temperature_elevator-1].temperature_vector=DOWN;
			}
			else
			{
				el[temperature_elevator-1].temperature_vector=STOP;
			}
			l=83;
			gotoxy(l,k);
			printf("                                      ");			
			break;
		}
	}
}
void eraser()//지우개 함수. 코드 구현 시 편리함을 위해
{
	l=83;
	k=27;
	for(int m=0;m<6;m++)
	{
		gotoxy(l,k);
		printf("                                                    ");
		k++;
	}
	l=95;
	k=26;
}
