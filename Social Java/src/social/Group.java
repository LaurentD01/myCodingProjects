package social;

import java.util.List;

import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.Id;
import jakarta.persistence.ManyToMany;
import jakarta.persistence.Table;

@Entity
@Table(name = "gruppi")
class Group {
    @Id
    private String name;
    
    @ManyToMany(fetch = FetchType.EAGER)
    private List<Person> partecipanti;

    public Group() {
        
    }

    public Group(String nomeGruppo){
        this.name = nomeGruppo;
    }

    public String getName(){
        return name;
    }

    public List<Person> getMembers(){
        return partecipanti;
    }

    public void setName(String nuovoNome){
        this.name = nuovoNome;
    }

    public void addPerson(Person p){
        partecipanti.add(p);
    }
}
