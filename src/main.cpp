#include "funcs.hpp"

//void tstf(mpc_t z,size_t p,mpc_t c){
//    mpc_pow_ui(z,z,p,MPC_RNDZZ);
//    mpc_add(z,z,c,MPC_RNDZZ);
//}

int main(){

    double sc=2;
    const size_t ws=500;
    size_t ss=ws,itc=24;
    bool clicked=1,shrink=0;

    ///Set display
    sf::RenderWindow window(sf::VideoMode(ws,ws),"1.0.0",sf::Style::Titlebar|sf::Style::Close);
    sf::Event event;
    window.setMouseCursorVisible(0);

    ///Set selection
    sf::RectangleShape selection;
    selection.setOutlineThickness(2);
    selection.setOutlineColor(sf::Color::Red);
    selection.setFillColor(sf::Color::Transparent);

    sf::RectangleShape mid;
    mid.setOutlineThickness(1);
    mid.setOutlineColor(sf::Color::Magenta);
    mid.setFillColor(sf::Color::Transparent);
    mid.setSize({1,1});

    ///Set set
    sf::Texture texture;
    texture.create(ws,ws);
    sf::Sprite sprite(texture);
    sf::Uint32 parr[ws*ws];

    ///Set complex numbers
    double er;
    size_t prec=128,p=2;
    mpc_t bcrd,ccrd,buf,fz,c;
    mpfr_t absv;
    mpc_init2(c,64);
    mpc_init2(bcrd,prec);
    mpc_init2(ccrd,prec);
    mpfr_init2(absv,64);
    mpc_init2(buf,prec);
    mpc_init2(fz,prec);
    mpc_set_si_si(bcrd,-2,-2,MPC_RNDZZ);
    mpfr_set_si(R(buf),4,MPFR_RNDZ); //initial size

    ///Declare func
    std::stack<node> func;

    std::cout<<"Enter f(z) and escape radius: ";
    {
        std::string str;
        std::cin>>str>>er;
        tokenize(postfix(preproc(str)),func);
    }

    sf::Mouse::setPosition({ws/2,ws/2},window);

    while(window.isOpen()){
        while(window.pollEvent(event)) switch(event.type){
        case sf::Event::KeyPressed:
            switch(event.key.code){
            case sf::Keyboard::Escape:
                window.close();
                break;

            case sf::Keyboard::I:
                std::cout<<"Enter the new max iteration count: ";
                std::cin>>itc;
                break;

            case sf::Keyboard::S:
                std::cout<<"Enter the new shrinking multiplier: ";
                std::cin>>sc;
                break;

            case sf::Keyboard::E:
                std::cout<<"Enter the new escape radius: ";
                std::cin>>er;

                break;

            default:;
            }
            break;

        case sf::Event::Closed:
            window.close();
            break;

        case sf::Event::MouseButtonReleased:
            if(event.mouseButton.button==sf::Mouse::Right) shrink=1;
            clicked=1;
            break;

        case sf::Event::MouseWheelScrolled:
            ss+=event.mouseWheelScroll.delta*5;
            if(ss<5) ss=5;
            break;

        default:;
        }
        ///Process data
        if(clicked){
            double umx=(double(sf::Mouse::getPosition(window).x)-ss/2)/ws,umy=(double(sf::Mouse::getPosition(window).y)-ss/2)/ws;
            if(umx>=0&&umy>=0&&umx+double(ss)/ws<=1&&umy+double(ss)/ws<=1){
                ///Adjust precision
                prec+=(shrink?-log2(sc):log2(ws/ss))+1;
                mpc_set_prec(ccrd,prec);
                mpc_set_prec(fz,prec);
                mpfr_set_prec(absv,prec);

                mpc_set(fz,bcrd,MPC_RNDZZ);
                mpc_set_prec(bcrd,prec);
                mpc_set(bcrd,fz,MPC_RNDZZ);

                mpc_set(fz,buf,MPC_RNDZZ);
                mpc_set_prec(buf,prec);
                mpc_set(buf,fz,MPC_RNDZZ);

                ///Get bcrd
                if(shrink){
                    mpfr_mul_d(I(buf),R(buf),sc/2-0.5,MPFR_RNDN);
                    mpfr_sub(R(bcrd),R(bcrd),I(buf),MPFR_RNDN);
                    mpfr_sub(I(bcrd),I(bcrd),I(buf),MPFR_RNDN);
                }else{
                    mpfr_mul_d(I(buf),R(buf),umx,MPFR_RNDN);
                    mpfr_add(R(bcrd),R(bcrd),I(buf),MPFR_RNDN);

                    mpfr_mul_d(I(buf),R(buf),umy,MPFR_RNDN);
                    mpfr_add(I(bcrd),I(bcrd),I(buf),MPFR_RNDN);
                }

                mpc_set(ccrd,bcrd,MPC_RNDZZ);

                ///Get size
                mpfr_mul_d(R(buf),R(buf),shrink?sc:double(ss)/ws,MPFR_RNDN);

                ///Get step
                mpfr_div_ui(I(buf),R(buf),ws,MPFR_RNDN);

                size_t tmp=0;
                ///Draw the set
                memset(parr,0,sizeof(parr));
                for(size_t scry=0;scry<ws;mpfr_add(I(ccrd),I(ccrd),I(buf),MPFR_RNDZ),scry++){
                    for(size_t scrx=0;scrx<ws;mpfr_add(R(ccrd),R(ccrd),I(buf),MPFR_RNDZ),scrx++){
                        if(!(++tmp%10000)) std::cout<<tmp/1000<<"K\n";
                        ///Test convergence
                        mpc_set(fz,ccrd,MPC_RNDZZ);

                        size_t i=-1;
                        while(++i<itc){
                            //tstf(fz,p,c);
                            eval(func,fz);
                            mpc_abs(absv,fz,MPFR_RNDZ);
                            if(mpfr_cmp_d(absv,6)>0) break;
                        }
                        i=255-i*255/itc;
                        parr[scry*ws+scrx]=((255<<24)|(i<<16)|(i<<8)|(i));
                    }
                    mpfr_set(R(ccrd),R(bcrd),MPFR_RNDZ);
                }
                texture.update(reinterpret_cast<sf::Uint8*>(parr));
            }
            clicked=0;
            shrink=0;
        }

        window.clear();

        window.draw(sprite);

        selection.setSize({ss,ss});
        selection.setPosition({float(sf::Mouse::getPosition(window).x)-ss/2,float(sf::Mouse::getPosition(window).y)-ss/2});
        window.draw(selection);

        if(ss>=20){
            mid.setPosition({float(sf::Mouse::getPosition(window).x),float(sf::Mouse::getPosition(window).y)});
            window.draw(mid);
        }

        window.display();
    }

    ///Clear dynamically allocated arrays
    mpc_clear(c);
    mpc_clear(bcrd);
    mpc_clear(ccrd);
    mpfr_clear(absv);
    mpc_clear(buf);
    mpc_clear(fz);

    return 0;
}
